obj-m += the_hr_sleep.o
the_hr_sleep-objs += hr_sleep.o ./lib/vtpmo.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

