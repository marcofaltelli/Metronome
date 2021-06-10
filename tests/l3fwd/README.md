# l3fwd
This directory contains the l3fwd example. Please use this example only once you have completed the prerequisites part inserted in the README in the root directory.
## Prerequisites
Please check these two parameters in ```l3fwd.h``` before compiling:
* In case your hr_sleep syscall number is **not** 134, please update the ```SYSCALL_ENTRY``` macro with the number of your hr_sleep syscall.
* Please also update the CPU_FREQ macro with your CPU nominal frequency (in GHz)
* 🆕 In order to compute & output metrics in a json file (see [this](https://gitlab.com/lalleanz/metronome/-/tree/multiqueue/tests/l3fwd#-metrics-output-file) section for format description), please compile with `#define STATS 1` (can be found in `l3fwd.h`)
## Precautions
* Please be sure that the cores in which you are running Metronome are located on the same NUMA node of your interfaces.
* Verify that your CPU power governor is not set to ```powersave```, but rather to  ```performance```.
* If possible, try to use isolated cores.
* We already configured the l3fwd example so as to forward traffic with IP prefix 10.0.0.0/16 to interface 1. This rule will be matched by the packets sent by MoonGen, so please make sure that this configuration is followed while running the experiment. Otherwise, you can add your own routes at line 87 of the ```l3fwd_lpm_timer.c``` file.
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
As an example, we may want to run l3fwd on cores 7,9,11 with interfaces 5e:00.0 and 5e:00.1 (interface 5e:00.0 is acting as the RX interface) and with a vacation period V of 10'000 ns. We then write the following code
```(bash)
$ sudo ./build/l3fwd -l 7,9,11 -n 3 -w 5e:00.0 -w 5e:00.1 -- -P -L -p 0x03 --config '(0,0,7),(0,0,9),(0,0,11)' -V 10000 -m 1
```
The test layout is illustrated as follows:

In our example, interface 5e:00.0 (which is interface 0 in the portmask) acts as Metronome's RX interface. Please make sure that your interfaces list follows the ascending order (You can get the ordered list running the ```./usertools/dpdk-devbind.py --status``` command).

Metronome on our servers achieves 0% packet loss with a V value of 10'000 ns using 3 isolated cores. We do not ensure that on a different server the same situation can be repeated, in that case please try to reduce the V value or use isolated cores.

## 🆕 Metrics output file
The resulting metrics are written in a json formatted file named `<n_cores>_<n_queues>_<vacant_period>.json`, where `<n_cores>` are the number of cores assigned to Metronome, `<n_queues>` the number of queues used and `<vacant_period>` is the target vacant period in ns.

An example json (`4_4_1500.json`) is:

```json
{
  "num_cores": 4,
  "num_queues": 4,
  "tot_exec_time_s": 2.5364007066666665,
  "tot_cpu_time_s": 1,
  "cpu_usage_perc": 39.42594706631344,
  "energy_uj": 81097998,
  "power_w": 31.973653763319934,
  "tot_free_tries": 559153,
  "tot_busy_tries": 0,
  "per_queue_stats": [
    {
      "vacant_period_us": 8.53545081790103,
      "busy_period_us": 0.017284496442939525,
      "per_core_busy_tries": [
        139944,
        0,
        0,
        0
      ],
      "per_core_lock_successes": [
        139944,
        0,
        0,
        0
      ]
    },

    ... 
   
    {
      "vacant_period_us": 8.543540802990535,
      "busy_period_us": 0.0223301402868972,
      "per_core_busy_tries": [
        0,
        0,
        0,
        139769
      ],
      "per_core_lock_successes": [
        0,
        0,
        0,
        139769
      ]
    }
  ]
}
```
