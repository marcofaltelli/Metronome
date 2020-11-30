# Metronome: adaptive packet retrieval in DPDK
Welcome to the official Metronome repository: Metronome will be presented in the next weeks at CoNEXT 2020.

Slides of the presentation and a video will be soon available.

We encourage you to clone Metronome and try it on your Linux machine. You can contact us using [our Slack channel](https://metronome-dpdk.slack.com) for any questions or trouble in using Metronome.

If you are looking for the CoNEXT artifacts evaluation, please switch to the ```artifacts``` branch.

The repository is organized with two main directories:
* **hr_sleep:** contains the kernel module to load the hr_sleep patch and a basic test to verify its correct working and the performance gain compared to Linux' nanosleep().
*  **tests:** contains the tests explained in Section 5.5 of the paper. We encourage reviewers to use the **l3fwd** experiment as it is the one used for evaluation in Section 5. 
# Prerequisites
The hr_sleep module was tested on Linux kernels ranging from 3.2 to 5.8. If you're having issues with other kernel versions, please contact us.

This guide is intended for Intel x86 CPUs only.
## Isolate CPUs
Our tests have been executed with Metronome running on isolated CPUs. Although this step is not strictly necessary, we recommend it in order to obtain results which are more reproducible and also closer to the evaluation. This [video](https://www.youtube.com/watch?v=FGVryuQRkOg) shows how to isolate CPUs in Ubuntu. If you wish to isolate your CPUs, please execute this step first since it will imply a machine reboot.
## DPDK 19.11
We used the DPDK 19.11.1 LTS stable release which can be downloaded [here](http://static.dpdk.org/rel/dpdk-19.11.1.tar.gz). Different releases of the DPDK 19.11 LTS release should work as well.

Once downloaded, uncompress the archive and move to the uncompressed DPDK directory:
```(bash)
$ tar xJf dpdk-19.11.1.tar.xz
$ cd dpdk-19.11.1
```
Before compiling DPDK install the required dependencies (for Ubuntu/Debian systems, for other systems please refer to the official guide [here](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html#compilation-of-the-dpdk)):
```(bash)
$ sudo apt-get update
$ sudo apt-get install git build-essential linux-headers-`uname -r`
```
Now compile with:
```(bash)
$ make config T=x86_64-default-linuxapp-gcc
$ make
$ make install
```
After DPDK is successfully compiled, please insert the *igb_uio* driver doing
```(bash)
$ sudo modprobe uio
```
Then from your root DPDK directory do:
```
$ sudo insmod build/kmod/igb_uio.ko
```
## Moongen
We used Moongen to generate high traffic of packets.

Clone the Moongen repository with:
```(bash)
$ git clone https://github.com/emmericp/MoonGen.git
```
Then install the required dependencies:
```(bash)
$ sudo apt-get install -y build-essential cmake linux-headers-`uname -r` pciutils libnuma-dev
```
Finally build Moongen with:
```(bash)
$ cd MoonGen
$ ./build.sh
```
Please note that the setup done by MoonGen may cause loss of connectivity, as MoonGen tries to bind to the DPDK driver all of the inactive interfaces.
Using Moongen allows you also to reproduce the experiments used by the **software-switches** repo for evaluation.
For our tests, we used commit ```525d991``` of the ```master``` branch.
## software-switches
You can clone the software-switches repo [here](https://github.com/ztz1989/software-switches/tree/artifacts). For our purposes, only the ```moongen/``` directory is needed. There's no need to compile or setup anything.
We used commit ```5af2439``` of the ```artifacts``` branch.
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
We used the experiments inside the ```software-switches/moongen``` repo, in particular ```unidirectional-test.sh``` for throughput tests and ```latency-test.sh``` for latency measurements. 

These test must be started only after the Metronome ```l3fwd``` example is running. Please refer to the README in the ```tests/l3fwd``` directory in order to make Metronome run correctly.

Before starting them, please modify these .sh files in order to specify in the ```MOONGEN_DIR``` parameter your Moongen installation path. Please also modify your ```dpdk-conf.lua``` script in order to specify the cores where to run Moongen, the interfaces to be used and the memory allocation.

**N.B.** please specify your interfaces in the same order you see them through the ```./usertools/dpdk-devbind.py --status``` command (that is, ascending order). The first interface you specify will likely be used for sending, and the second one for receiving.

You can switch now to the ```examples/l3fwd``` directory to test our main example.

