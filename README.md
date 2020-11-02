# Metronome Artifacts repository
Welcome to the Metronome repository for CoNEXT 2020 artifacts evaluation.

We provide the necessary code and documentation to test Metronome on your commodity servers 
and replicate our experiments.

The repository is organized with two main directories:
* **hr_sleep:** contains the kernel module to load the hr_sleep patch and a basic test to verify its correct working and the performance gain compared to Linux' nanosleep().
*  **tests:** contains the tests explained in Section 5.5 of the paper. We encourage reviewers to use the **l3fwd** experiment as it is the one used for evaluation in Section 5. 
# Prerequisites
## DPDK 19.11
We used the DPDK 19.11.1 LTS stable release which can be downloaded [here](http://static.dpdk.org/rel/dpdk-19.11.1.tar.gz). Different releases of the DPDK 19.11 LTS release should work as well.
Once extracted, DPDK can be compiled following [this guide](https://doc.dpdk.org/guides/linux_gsg/build_dpdk.html). 
After DPDK is successfully compiled, please insert the *igb_uio* driver doing
```(bash)
$ sudo modprobe uio
$ sudo insmod kmod/igb_uio.ko
```
once switched to your DPDK ```build/``` directory.
## Moongen
We used Moongen to generate high traffic of packets. Moongen can be downloaded [here](https://github.com/emmericp/MoonGen). Please follow Its README to install it on your server.
Using Moongen allows you also to reproduce the experiments used by the **software-switches** repo for evaluation.
## software-switches
You can clone the software-switches repo [here](https://github.com/ztz1989/software-switches/tree/artifacts). For our purposes, only the ```moongen/``` directory is needed.
# Testbed setup
## Loading the hr_sleep module
Switch to the **hr_sleep** directory and follow the instructions in the directory's README file.
## Interfaces binding
Please make sure that all of the interfaces you wish to use are bound to the DPDK driver.
To see the status of all ports of your network system, run:
```(bash)
$ cd YOUR_DPDK_HOME
$ ./usertools/dpdk-devbind.py --status

Network devices using DPDK-compatible driver
============================================
0000:82:00.0 '82599EB 10-GbE NIC' drv=igb_uio unused=ixgbe
0000:82:00.1 '82599EB 10-GbE NIC' drv=igb_uio unused=ixgbe

Network devices using kernel driver
===================================
0000:04:00.0 'I350 1-GbE NIC' if=em0  drv=igb unused=igb_uio *Active*
0000:04:00.1 'I350 1-GbE NIC' if=eth1 drv=igb unused=igb_uio
0000:04:00.2 'I350 1-GbE NIC' if=eth2 drv=igb unused=igb_uio
0000:04:00.3 'I350 1-GbE NIC' if=eth3 drv=igb unused=igb_uio

Other network devices
=====================
<none>
```
To bind interface ```eth1``` (with PCI address ```04:00.1```) to the ```igb_uio``` driver:
```(bash)
$ ./usertools/dpdk-devbind.py --unbind 04:00.1
$ ./usertools/dpdk-devbind.py --bind=igb_uio 04:00.1
```
# Running experiments
We used the experiments inside the ```software-switches/moongen``` repo, in particular ```unidirectional-test.sh``` for throughput tests and ```latency-test.sh``` for latency measurements. Before starting them, please modify these .sh files in order to specify in the ```MOONGEN_DIR``` parameter your Moongen installation path. Please also modify your ```dpdk-conf.lua``` script in order to specify the cores where to run Moongen, the interfaces to be used and the memory allocation.

**N.B.** please specify your interfaces in the same order you see them through the ```./usertools/dpdk-devbind.py --status``` command (that is, ascending order). The first interface you specify will likely be used for sending, and the second one for receiving.

You can switch now to the ```examples/l3fwd``` directory to test our main example.
