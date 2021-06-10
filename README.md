# Metronome: adaptive packet retrieval in DPDK
Welcome to the official Metronome repository: Metronome enables CPU usage **proportional** to the incoming load in **DPDK**, as well as **flexible** sharing of **CPU resources** between I/O tasks and applications. Here are some pointers to deep into our work:
* Our **CoNEXT** paper is available [here](https://dl.acm.org/doi/pdf/10.1145/3386367.3432730).
* An **extension** of this work, comprehensive of a multiqueue orchestration algorithm and tests on 40Gbps NICs is available on [arXiv](https://arxiv.org/pdf/2103.13263.pdf).
* We also have a [**video**](https://dl.acm.org/doi/abs/10.1145/3386367.3432730#sec-supp) of the CoNEXT presentation.

We encourage you to clone Metronome and try it on your Linux machine. You can contact us using [our Slack channel](https://metronome-dpdk.slack.com) for any questions or trouble in using Metronome.

If you are looking for the CoNEXT artifacts evaluation, please switch to the ```artifacts``` branch.

The repository is organized with two main directories:
* **hr_sleep:** contains the kernel module to load the hr_sleep patch and a basic test to verify its correct working and the performance differences compared to Linux' nanosleep().
*  **tests:** contains the tests explained in Section 5.5 of the paper. We encourage reviewers to use the **l3fwd** experiment as it is the one used for the paper evaluation. 
# Prerequisites
Below is a description of the different testbeds on which we tested Metronome. We do not guarantee Metronome will work smoothly with any different configuration
* **CPU**: Intel x86
* **Kernel**: Linux kernel from 3.2 to 5.8
* **NICs**: Intel X520, X710, XL710

Software requisites:
* **DPDK**: version 19.11


# Testbed setup
## Loading the hr_sleep module
Switch to the **hr_sleep** directory if you're interested in mounting our hr_sleep sleep service and follow the instructions in the directory's README file. This is not mandatory for Metronome to work.
## Testing
You can switch now to the ```examples/l3fwd``` directory to test our main example.

