###########################################################################
#
# Local source generation targets.
#
###############################################################################

krn = bf_kdrv

ifneq ($(KERNELRELEASE),)

obj-m := $(krn).o

else

KDIR ?= /lib/modules/$$(uname -r)/build
ARCH ?= x86

all: default

default:
	            $(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$$BUILD_DIR src=$$PWD modules

clean:
	            $(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$$BUILD_DIR src=$$PWD clean

.PHONY : clean
endif
