FJSEC_DEPS_MOD_LIST := $(INSTALLED_KERNEL_TARGET) \
    kmodules
FJSEC_MAKE_HEADER := kernel/security/fjsec_make_header.sh

.PHONY: fjsec_rebuild_kernel

fjsec_rebuild_kernel: $(FJSEC_DEPS_MOD_LIST)
	@echo "FJSEC make header, and kernel rebuild."
	$(FJSEC_MAKE_HEADER) $(TARGET_OUT)
	+$(hide) $(kernel-make) zImage
	$(ACP) $(BUILT_KERNEL_TARGET) $(INSTALLED_KERNEL_TARGET)
