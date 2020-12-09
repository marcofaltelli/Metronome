/*
* 
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This module is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* @file hrsleep.c 
* @brief This is the main source for the Linux Kernel Module which implements
* 	 the runtime discovery of the syscall table position and of free entries (those 
* 	 pointing to sys_ni_syscall). After the discovery, the hr_sleep syscall is installed.
*
* @author Francesco Quaglia, Marco Faltelli, Giacomo Belocchi
*
* @date November 22, 2020
*/

#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <linux/syscalls.h>
#include "./include/vtpmo.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <framcesco.quaglia@uniroma2.it>, Giacomo Belocchi <giacomo.belocchi@uniroma2.it, Marco Faltelli <marco.faltelli@uniroma2.it>>");
MODULE_DESCRIPTION("hr_sleep");



#define MODNAME "HR_SLEEP"


extern int sys_vtpmo(unsigned long vaddr);


#define ADDRESS_MASK 0xfffffffffffff000//to migrate

#define START 			0xffffffff00000000ULL		// use this as starting address --> this is a biased search since does not start from 0xffff000000000000
#define MAX_ADDR		0xfffffffffff00000ULL
#define FIRST_NI_SYSCALL	134
#define SECOND_NI_SYSCALL	174
#define THIRD_NI_SYSCALL	182 
#define FOURTH_NI_SYSCALL	183
#define FIFTH_NI_SYSCALL	214	
#define SIXTH_NI_SYSCALL	215	
#define SEVENTH_NI_SYSCALL	236	

#define ENTRIES_TO_EXPLORE 256


unsigned long *hacked_ni_syscall=NULL;
unsigned long **hacked_syscall_tbl=NULL;

unsigned long sys_call_table_address = 0x0;
module_param(sys_call_table_address, ulong, 0660);

unsigned long sys_ni_syscall_address = 0x0;
module_param(sys_ni_syscall_address, ulong, 0660);


int good_area(unsigned long * addr){

	int i;
	
	for(i=1;i<FIRST_NI_SYSCALL;i++){
		if(addr[i] == addr[FIRST_NI_SYSCALL]) goto bad_area;
	}	

	return 1;

bad_area:

	return 0;

}



/* This routine checks if the page contains the begin of the syscall_table.  */
int validate_page(unsigned long *addr){
	int i = 0;
	unsigned long page 	= (unsigned long) addr;
	unsigned long new_page 	= (unsigned long) addr;
	for(; i < PAGE_SIZE; i+=sizeof(void*)){		
		new_page = page+i+SEVENTH_NI_SYSCALL*sizeof(void*);
			
		// If the table occupies 2 pages check if the second one is materialized in a frame
		if( 
			( (page+PAGE_SIZE) == (new_page & ADDRESS_MASK) )
			&& sys_vtpmo(new_page) == NO_MAP
		) 
			break;
		// go for patter matching
		addr = (unsigned long*) (page+i);
		if(
			   ( (addr[FIRST_NI_SYSCALL] & 0x3  ) == 0 )		
			   && (addr[FIRST_NI_SYSCALL] != 0x0 )			// not points to 0x0	
			   && (addr[FIRST_NI_SYSCALL] > 0xffffffff00000000 )	// not points to a locatio lower than 0xffffffff00000000	
	//&& ( (addr[FIRST_NI_SYSCALL] & START) == START ) 	
			&&   ( addr[FIRST_NI_SYSCALL] == addr[SECOND_NI_SYSCALL] )
			&&   ( addr[FIRST_NI_SYSCALL] == addr[THIRD_NI_SYSCALL]	 )	
			&&   ( addr[FIRST_NI_SYSCALL] == addr[FOURTH_NI_SYSCALL] )
			&&   ( addr[FIRST_NI_SYSCALL] == addr[FIFTH_NI_SYSCALL] )	
			&&   ( addr[FIRST_NI_SYSCALL] == addr[SIXTH_NI_SYSCALL] )
			&&   ( addr[FIRST_NI_SYSCALL] == addr[SEVENTH_NI_SYSCALL] )	
			&&   (good_area(addr))
		){
			hacked_ni_syscall = (void*)(addr[FIRST_NI_SYSCALL]);				// save ni_syscall
			sys_ni_syscall_address = (unsigned long)hacked_ni_syscall;
			hacked_syscall_tbl = (void*)(addr);				// save syscall_table address
			sys_call_table_address = (unsigned long) hacked_syscall_tbl;
			return 1;
		}
	}
	return 0;
}

/* This routines looks for the syscall table.  */
void syscall_table_finder(void){
	unsigned long k; // current page
	unsigned long candidate; // current page

	for(k=START; k < MAX_ADDR; k+=4096){	
		candidate = k;
		if(
			(sys_vtpmo(candidate) != NO_MAP) 	
		){
			// check if candidate maintains the syscall_table
			if(validate_page( (unsigned long *)(candidate)) ){
				printk("%s: syscall table found at %px\n",MODNAME,(void*)(hacked_syscall_tbl));
				printk("%s: sys_ni_syscall found at %px\n",MODNAME,(void*)(hacked_ni_syscall));
				break;
			}
		}
	}
	
}


#define MAX_FREE 15
int free_entries[MAX_FREE];
module_param_array(free_entries,int,NULL,0660);//default array size already known - here we expose what entries are free


#define NO (0)
#define YES (NO+1)
#define AUDIT if(0)


typedef struct _control_record{
        struct task_struct *task;
        int pid;
        int awake;
        struct hrtimer hr_timer;
} control_record;


static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer ){

        control_record *control;
        struct task_struct *the_task;

        control = (control_record*)container_of(timer,control_record, hr_timer);
        control->awake = YES;
        the_task = control->task;
        wake_up_process(the_task);

        return HRTIMER_NORESTART;
}



#define SYS_CALL_INSTALL

#ifdef SYS_CALL_INSTALL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(2, _goto_sleep, unsigned long, nanosecs, unsigned long, tries){
#else
asmlinkage long sys_goto_sleep(unsigned long nanosecs, unsigned long tries){
#endif

        DECLARE_WAIT_QUEUE_HEAD(the_queue);//here we use a private queue 
        control_record* control;
        ktime_t ktime_interval;

        AUDIT
        printk("%s: thread %d requests a sleep for %lu nanosecs on %lu tries\n",MODNAME,current->pid,nanosecs,tries);

       // if(nanosecs == 0) return 0;

        AUDIT
        printk("%s: thread %d going to sleep for %lu nanosecs\n",MODNAME,current->pid,nanosecs);

        ktime_interval = ktime_set( 0, nanosecs );

        control = (control_record*)((void*)current->stack + sizeof(struct thread_info));

        hrtimer_init(&(control->hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);

        control->hr_timer.function = &my_hrtimer_callback;

redo:
        control->task = current;
        control->pid  = control->task->pid; //current->pid is more costly
        control->awake = NO;

        hrtimer_start(&(control->hr_timer), ktime_interval, HRTIMER_MODE_REL);


        wait_event_interruptible(the_queue, control->awake == YES);

        hrtimer_cancel(&(control->hr_timer));

        AUDIT
        printk("%s: thread %d exiting usleep\n",MODNAME, current->pid);

	tries--;
	if(tries>0) goto redo;

        return 0;

}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_goto_sleep = (unsigned long) __x64_sys_goto_sleep;	
#else
#endif

unsigned long cr0;

static inline void
write_cr0_forced(unsigned long val)
{
    unsigned long __force_order;

    /* __asm__ __volatile__( */
    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
}

static inline void
protect_memory(void)
{
    write_cr0_forced(cr0);
}

static inline void
unprotect_memory(void)
{
    write_cr0_forced(cr0 & ~X86_CR0_WP);
}

#else
#endif



int init_module(void) {
	
	int i,j;
		
        printk("%s: initializing\n",MODNAME);
	
	syscall_table_finder();

	if(!hacked_syscall_tbl){
		printk("%s: failed to find the sys_call_table\n",MODNAME);
		return -1;
	}

	j=0;
	for(i=0;i<ENTRIES_TO_EXPLORE;i++)
		if(hacked_syscall_tbl[i] == hacked_ni_syscall){
			printk("%s: found sys_ni_syscall entry at syscall_table[%d]\n",MODNAME,i);	
			free_entries[j++] = i;
			if(j>=MAX_FREE) break;
		}

#ifdef SYS_CALL_INSTALL
	cr0 = read_cr0();
    unprotect_memory();
    hacked_syscall_tbl[FIRST_NI_SYSCALL] = (unsigned long*)sys_goto_sleep;
    protect_memory();
	printk("%s: a sys_call with 2 parameters has been installed as a trial on the sys_call_table at displacement %d\n",MODNAME,FIRST_NI_SYSCALL);	
#else
#endif

    printk("%s: module correctly mounted\n",MODNAME);

    return 0;

}

void cleanup_module(void) {
                
#ifdef SYS_CALL_INSTALL
	cr0 = read_cr0();
        unprotect_memory();
        hacked_syscall_tbl[FIRST_NI_SYSCALL] = (unsigned long*)hacked_ni_syscall;
        protect_memory();
#else
#endif
        printk("%s: shutting down\n",MODNAME);
        
}
