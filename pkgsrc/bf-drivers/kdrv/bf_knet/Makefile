###########################################################################
#
# Local source generation targets.
#
###############################################################################

krn = bf_knet

ifneq ($(KERNELRELEASE),)

obj-m := $(krn).o

ccflags-y := -I$(src)/../../include
ccflags-y += -I$(inc)
ccflags-y += -I$(src)

else

KDIR ?= /lib/modules/$$(uname -r)/build

all: default

default:
		$(MAKE) modules

clean:
		$(MAKE) clean

.PHONY : clean
endif



