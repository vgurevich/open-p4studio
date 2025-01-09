###########################################################################
#
# Local source generation targets.
#
###############################################################################

krn ?= bf_tun

ifneq ($(KERNELRELEASE),)

obj-m := $(krn).o

else

KDIR ?= /lib/modules/$$(uname -r)/build

all: default

default:
	            $(MAKE) -C $(KDIR) M=$$BUILD_DIR src=$$PWD modules

clean:
	            $(MAKE) -C $(KDIR) M=$$BUILD_DIR src=$$PWD clean

.PHONY : clean
endif
