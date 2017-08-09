# Display color driver Makefile

PWD     := $(shell pwd)
INCSRC = drivers/color

ccflags-y := -I$(INCSRC)

obj-$(CONFIG_DISPLAY_COLOR_DRV) += display_color_drv.o

hdmidrv-objs  += display_color_drv.o

