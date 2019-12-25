#########################
####### TARGET_DEV ######
#########################
CPU_ARCH=$(shell lscpu | grep "Architecture" | awk '{print $$2}')
CPU_CORE_NUM=$(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)


TARGET_DEV_TEMP=VM


PWD=$(shell pwd)
TAPPLET_TARGET_DEV=$(TARGET_DEV_TEMP)
TAPPLET_RELEASE_DIR=$(PWD)/release/$(TARGET_DEV_TEMP)
$(shell mkdir -p $(TAPPLET_RELEASE_DIR))

ifeq ($(TARGET_DEV),)
export TARGET_DEV=$(TARGET_DEV_TEMP)
endif

#########################
###### VPP_SRC_DIR ######
#########################
ifeq ($(VPP_SRC_DIR),)
TEMPVALUE=$(shell if [ -e /vpp1810 ]; then echo "export"; else echo "noexist"; fi;)
endif

ifeq ($(TEMPVALUE), export)
export VPP_SRC_DIR=/vpp1810
else ifeq ($(TEMPVALUE), noexist)
$(warning /vpp1810 not exist , create it (ln -s <vpp1810> /vpp1810))
endif



.PHONY : pre_make_test
pre_make_test:
	@echo $(CPU_ARCH)
	@echo $(CPU_CORE_NUM)
	@echo $(TARGET_DEV)
	@echo $(VPP_SRC_DIR)
	@echo $(TEMPVALUE)