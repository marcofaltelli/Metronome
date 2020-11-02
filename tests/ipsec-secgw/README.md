# Metronome - ipsec-secgw
This folder contains the ipsec-gw example.
## Compiling
Follow the instructions at [this link](https://doc.dpdk.org/guides-19.11/sampleappug/ipsecsecgw.html) for installing the requirements for the application.
Compile using make Then launch with 
```(bash)
./build/ipsec-secgw -l 11,7,9 -n 4 --socket-mem 0,2048 --vdev "crypto_null" -- -p 0xf -P -u 0x2 --config '(0,0,9),(0,0,7),(0,0,11)' -f test.cfg
```
