# hr_sleep
This repository shows how to insert the hr_sleep syscall module and provides an example to verify the performance gains of hr_sleep.
## Setup
Compile the module with ```make```, then load it by doing:
```(bash)
$ sudo insmod the_usctm.ko
```
Then use the ```dmesg``` command to check that the module was mounted correctly and the new syscall number. Usually, the kernel will use entry 134 for inserting the hr_sleep() system call. If this does not happen, please keep in mind the syscall number.

We've tested the module insertion on Linux kernel 5.4. Older version are expected to work too.
## Run the example
You can switch to the ```user/``` directory and try an example.
First, update the ```float scale``` parameter inside the ```nanosleep-user.c``` source file with your CPU nominal frequency (in our case, it was 2.1GHz).

You can now compile and execute the example through 
```(bash)
$ gcc nanosleep-user.c
$ ./a.out <SYSCALL_NUM> <TIMEOUT IN NANOSECS>
```
For example, execute ```./a.out 134 1000``` if your hr_sleep has syscall index 134 and you want to compare hr_sleep() and nanosleep() with a 1000 ns timer granularity.

The example will output for both functions the number of clock cycles elapsed during their sleep period. 
