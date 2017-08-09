LOCAL_PATH:= $(call my-dir)

# HID plugin

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	device.c \
	fakehid.c \
	main.c \
	manager.c \
	server.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.93\" \
	-DSTORAGEDIR=\"/data/misc/bluetoothd\" \
	-DCONFIGDIR=\"/etc/bluetooth\"

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../btio \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../src \
	$(LOCAL_PATH)/../gdbus \
	$(call include-path-for, glib) \
	$(call include-path-for, dbus)

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd \
	libbluetooth \
	libbtio \
	libcutils \
	libdbus \
	libexpat \
	libglib 

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/bluez-plugin
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/bluez-plugin
LOCAL_MODULE := input

include $(BUILD_SHARED_LIBRARY)

#
# sixpair
#
# Enable sixpair only on Cardhu
ifeq ($(TARGET_DEVICE),cardhu)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= sixpair.c
LOCAL_MODULE:=sixpair
LOCAL_CFLAGS:= \
        -DVERSION=\"4.93\" -DSTORAGEDIR=\"/data/misc/bluetoothd\" -DNEED_PPOLL -D__ANDROID__

LOCAL_C_INCLUDES:=\
        $(LOCAL_PATH)/../lib \
        $(LOCAL_PATH)/../lib/bluetooth \
        $(LOCAL_PATH)/../src \
        $(LOCAL_PATH)/../gdbus \
        $(call include-path-for, glib) \
        $(call include-path-for, dbus) \
        vendor/nvidia/tegra/3rdparty/libusb/libusb

LOCAL_SHARED_LIBRARIES := \
        libbluetoothd \
        libbluetooth \
        libdbus \
        libexpat \
        libcutils \
        libglib \
        libc \
        libusb

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

endif
