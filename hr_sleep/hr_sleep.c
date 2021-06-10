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
* @file hr_sleep.c 
* @brief This is the main source for the Linux Kernel Module which implements
* 	 the runtime discovery of the syscall table position and of free entries (those 
* 	 pointing to sys_ni_syscall) - the only assumption is that we have access to the
* 	 compiled version of the kernel (even under randomization)
*	 
*	 an add on installs a precise sleep service with very fine granularity 
*
* @author Francesco Quaglia, Romolo Marotta
* @author (add on) Francesco Quaglia, Giacomo Belocchi, Marco Faltelli
*
* @date November 8, 2019
* @date February 8, 2020 (ad on)
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
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <francesco.quaglia@uniroma2.it>, Romolo Marotta <marotta@diag.uniroma1.it>, Giacomo Belocchi <giacomo.belocchi@uniroma2.it, Marco Faltelli <marco.faltelli@uniroma2.it>>");
MODULE_DESCRIPTION("hr_sleep and an add on");


#define MODNAME "hr_sleep"


extern int sys_vtpmo(unsigned long vaddr);


#define ADDRESS_MASK 0xfffffffffffff000//to migrate

#define START 			0xffffffff00000000ULL		// use this as starting address --> this is a biased search since does not start from 0xffff000000000000
#define MASK_12 		0x0000000000000fffULL		// mask to get least 12 bits
#define ALIGN_12_INC		(MASK_12+0x1ULL)		// value to iterate over entries of size equal to 2**12
#define MASK_21 		0x00000000001fffffULL		// mask to get least 21 bits
#define ALIGN_21_INC		(MASK_21+0x1ULL)		// value to iterate over entries of size equal to 2**21
#define MAX_ADDR		0xfffffffffff00000ULL
#define FIRST_NI_SYSCALL	134
#define SECOND_NI_SYSCALL	174
#define THIRD_NI_SYSCALL	177
#define FORTH_NI_SYSCALL	178

#define ENTRIES_TO_EXPLORE 256


unsigned long *hacked_ni_syscall=NULL;
unsigned long **hacked_syscall_tbl=NULL;



void syscall_table_finder(void){
	hacked_syscall_tbl = (void *) kallsyms_lookup_name("sys_call_table");
	printk("%s: syscall table found at %px\n",MODNAME,(void*)(hacked_syscall_tbl));
	return;
}



#define MAX_FREE 10
int free_entries[MAX_FREE];
module_param_array(free_entries,int,NULL,0660);//default array size already known - here we expose what entries are free

// This gives access to read_cr0() and write_cr0()
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h>
#endif
#ifndef X86_CR0_WP
#define X86_CR0_WP 0x00010000
#endif


#define NO (0)
#define YES (NO+1)


typedef struct _control_record{
        struct task_struct *task;
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _goto_sleep, long, nanoseconds){
#else
asmlinkage long sys_goto_sleep(long nanoseconds){
#endif

	DECLARE_WAIT_QUEUE_HEAD(the_queue);//here we use a private queue
	control_record control;
	ktime_t ktime_interval;
 
	if (nanoseconds < 0)
		return -EINVAL;
	if (nanoseconds == 0)
		return 0;

	ktime_interval = ktime_set(0, nanoseconds);
	hrtimer_init(&(control.hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	control.hr_timer.function = &my_hrtimer_callback;
	control.task = current;
	control.awake = NO;
	hrtimer_start(&(control.hr_timer), ktime_interval, HRTIMER_MODE_REL);

	wait_event_interruptible(the_queue, control.awake == YES);
	hrtimer_cancel(&(control.hr_timer));

	if (control.awake == 0)
		return -EINTR;
	return 0;
}

int target;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static unsigned long sys_goto_sleep = (unsigned long) __x64_sys_goto_sleep;	
#else
#endif

unsigned long cr0;

extern unsigned long __force_order ;
static inline void write_cr0_forced(unsigned long val)
{
	asm volatile("mov %0,%%cr0" : "+r"(val), "+m"(__force_order));
}


static inline void one_wp(void)
{
	write_cr0_forced(read_cr0() | 0x10000);
}

static inline void zero_wp(void)
{
	write_cr0_forced(read_cr0() & ~0x10000);
}

int init_module(void) {
	
	int i,j;
	unsigned long cr0;
	
        printk("%s: initializing\n",MODNAME);
	
	syscall_table_finder();

	hacked_ni_syscall = (void*)(hacked_syscall_tbl[FIRST_NI_SYSCALL]); 

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

	for(i=0;i<ENTRIES_TO_EXPLORE;i++)
		if(hacked_syscall_tbl[i] == hacked_ni_syscall){
			target = i;
			printk("%s: getting entry %d\n",MODNAME,target);	
			break;
		}

        cr0 = read_cr0();
        zero_wp();
	hacked_syscall_tbl[target] = (unsigned long*)sys_goto_sleep;
	one_wp();

        printk("%s: module correctly mounted\n",MODNAME);

        return 0;

}

void cleanup_module(void) {        
	unsigned long cr0;

        printk("%s: shutting down\n",MODNAME); 
        cr0 = read_cr0();
	zero_wp();
        hacked_syscall_tbl[target] = hacked_ni_syscall;
	one_wp();
        printk("%s: sys-call table restored to its original content\n",MODNAME);

}
