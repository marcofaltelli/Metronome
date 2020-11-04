# l3fwd
This directory contains the l3fwd example. Please use this example only once you have completed the prerequisites part inserted in the README in the root directory.
## Prerequisites
In case your hr_sleep syscall number is **not** 134, please update the ```SYSCALL_ENTRY``` macro in ```l3fwd_lpm_timer.c``` with the number of your hr_sleep syscall.
## Compile
You can now compile the l3fwd example with ```make l3fwd```.
## Run
Once compiled, you can run the example with
```(bash)
$ sudo ./build/l3fwd -l <CORE1, CORE2,...,COREn> -n 3 -w <IFACE0> -w <IFACE1> -- -P -L -p 0x03 --config '(0,0,CORE1),(0,0,CORE2),...,(0,0,COREn)' -V <V VALUE> -m 1
```
```-l``` specifies the list of cores on which DPDK will run
```-w``` specifies the whitelist of the interfaces to be used through their PCI address
```--config``` specifies the list of binding of NICs and their RX queues to the different cores. Specifically, it follows the path `(port,queue,core)`. In this example, we are using interface 0 as the RX NIC with an only RX queue and binding it to different cores
```-V``` specifies the target V (vacation period) value in nanoseconds
```-m``` specifies the sleep function to be used. 1 is for hr_sleep(), 0 is for nanosleep().
```-a``` is a legacy parameter and should be removed soon. Please use only ```-a 1```
As an example, we may want to run l3fwd on cores 7,9,11 with interfaces 5e:00.0 and 5e:00.1 (interface 5e:00.0 is acting as the RX interface) and with a vacation period V of 10'000 ns. We then write the following code
```(bash)
$ sudo ./build/l3fwd -l 7,9,11 -n 3 -w 5e:00.0 -w 5e:00.1 -- -P -L -p 0x03 --config '(0,0,7),(0,0,9),(0,0,11)' -V 10000 -m 1
```
The test layout is illustrated as follows:

In our example, interface 5e:00.0 (which is interface 0 in the portmask) acts as Metronome's RX interface. Please make sure that your interfaces list follows the ascending order (You can get the ordered list running the ```./usertools/dpdk-devbind.py --status``` command).
## Precautions
* Please be sure that the cores in which you are running Metronome are located on the same NUMA node of your interfaces.
* Verify that your CPU power governor is not set to ```powersave```, but rather to  ```performance``` or ```ondemand```.
* We already configured the l3fwd example so as to forward traffic with IP prefix 10.0.0.0/16 to interface 1. This rule will be matched by the packets sent by MoonGen, so please make sure that this configuration is followed while running the experiment. Otherwise, you can add your own routes at line 93 of the ```l3fwd_lpm_timer.c``` file.
* Metronome on our servers achieves 0% packet loss with a V value of 10'000 ns using 3 isolated cores. We do not ensure that on a different server the same situation can be repeated, in that case please try to reduce the V value.
