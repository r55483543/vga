#
TARGET = vga
ARCH= arm
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
KERNEL_ROOT = $(SOCEDS_ROOT)/embeddedsw/socfpga/sources/linux-socfpga-master/include
ARCH_ROOT = $(SOCEDS_ROOT)/embeddedsw/socfpga/sources/linux-socfpga-master/arch/$(ARCH)/include
PREBUILE_ROOT = $(SOCEDS_ROOT)/embeddedsw/socfpga/prebuilt_images/kernel_headers
CROSS_COMPILE = arm-linux-gnueabihf-
CFLAGS = -g -Wall   -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY)   -I$(HWLIBS_ROOT)/include/ -I$(PREBUILE_ROOT)/include/
#-I$(KERNEL_ROOT)/ -I$(ARCH_ROOT)/ -I$(ARCH_ROOT)/uapi/ -I$(KERNEL_ROOT)/uapi/

LDFLAGS =  -g -Wall 
CC = $(CROSS_COMPILE)gcc


build: $(TARGET)

$(TARGET): main.o 
	$(CC) $(LDFLAGS)   $^ -o $@ 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~