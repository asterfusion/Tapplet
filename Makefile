include pre_make.make

#export TARGET_DEV
ifeq ($(TARGET_DEV),)
$(error TARGET_DEV is not defined)
endif

ifeq ($(VPP_SRC_DIR),)
$(error VPP_SRC_DIR is not defined)
endif

CPU_CORE_NUM=$(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)
JOBS=$(shell expr 2 \* $(CPU_CORE_NUM) )

FULL_GITDATE=$(shell git log -1 --pretty=%cd --date=short)
VERSION=$(subst -,,$(FULL_GITDATE))
RELEASE=$(shell ver=`git log -n1 --pretty=%H`;echo $${ver:0:11})
GITDATE=$(shell echo $(FULL_GITDATE) | sed 's/-//g')

ifneq ($(shell uname),Darwin)
OS_ID        = $(shell grep '^ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
OS_VERSION_ID= $(shell grep '^VERSION_ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
endif

ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
PKG=deb
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
endif

# .PHONY: test
# test:
# 	@echo $(VERSION) $(RELEASE) 


SF_CMAKE_CONFIG_FILE=./build/sfcmake.config
define prepare_cmake_config
	@echo "" > $(SF_CMAKE_CONFIG_FILE)
	@echo "TARGET_DEVICE=$(TARGET_DEV)" >> $(SF_CMAKE_CONFIG_FILE)
	@echo "SF_VERSION=$(GITDATE)-$(RELEASE)" >> $(SF_CMAKE_CONFIG_FILE)
endef

all:
	make -C build -j$(JOBS)

.PHONY: help release debug clean  dist-clean build-release 
.PHONY: pkg-deb pkg-deb-release pkg-rpm pkg-rmp-release pkg-release

###################################
# help 
###################################
help:
	@echo "Make Targets:"
	@echo " install-dep         - install software dependencies "
	@echo " release             - execute release version config "
	@echo " debug               - execute debug version config "
	@echo " clean               - clear compiled results"
	@echo " dist-clean          - clear compiled results and config"
	@echo " build-release       - build release version config"
	@echo " build-debug         - build debug version config"
	@echo " install-plugin      - install only plugins to local host /usr/lib"
	@echo " pkg-release         - release packages to $(TAPPLET_RELEASE_DIR)"
	@echo " pkg-deb             - build DEB packages"
	@echo " pkg-deb-release     - release DEB packages to $(TAPPLET_RELEASE_DIR)"
	@echo " pkg-rpm             - build RPM packages"
	@echo " pkg-rpm-release     - release RPM packages to $(TAPPLET_RELEASE_DIR)"
	@echo ""
	@echo "Current Environment Values:"
	@echo " TARGET_DEV          = $(TARGET_DEV)"
	@echo " VPP_SRC_DIR         = $(VPP_SRC_DIR)"

APT_DEP = python3-pip
PYTHON3_DEP = setuptools pyinstaller  psutil
PYTHON3_DEP += requests requests-toolbelt

install-dep:
	sudo apt-get install $(APT_DEP)
	sudo pip3 install $(PYTHON3_DEP)


release:
	mkdir -p ./build
	$(call prepare_cmake_config)
	cd build ; cmake -L .. -DCMAKE_INSTALL_PREFIX:PATH=/usr/ -DCMAKE_BUILD_TYPE=Release 


debug:
	mkdir -p ./build
	$(call prepare_cmake_config)
	cd build ; cmake -L .. -DCMAKE_INSTALL_PREFIX:PATH=/usr/ -DCMAKE_BUILD_TYPE=Debug


clean:
	make -C build clean 
	make -C build-pkg clean
	make -C rest clean
	make -C sf_cli clean

dist-clean: clean
	make -C build-pkg dist-clean
	rm -rf build

build-release: release
	make -C build -j$(JOBS)

build-debug: debug
	make -C build -j$(JOBS)

#usually used after "make debug"
.PHONY: install-plugin 
install-plugin:
	cp ./build/lib/vpp_plugins/*.so /usr/lib/vpp_plugins/
	cp ./build/lib/libnshell.so /usr/lib/vpp_plugins/

###################################
# sf rest & sf cli 
###################################
.PHONY:  pkg-sf-cli  pkg-sfrest pre-pkg 


pkg-sf-cli:
	make -C sf_cli pkg-sf-rest-cli-bin  _VERSION=$(VERSION) _RELEASE=$(RELEASE)

pkg-sf-cli-release: pkg-sf-cli
	cp ./sf_cli/dist/*.tar* $(TAPPLET_RELEASE_DIR)

pkg-sfrest: 
	make -C rest 

pre-pkg: pkg-sf-cli pkg-sfrest 

###################################
# Debian 
###################################
pkg-deb: pre-pkg
	make -C build-pkg build-deb


pkg-deb-release: pkg-deb pkg-sf-cli-release
	cp build-pkg/install-native/*.deb $(TAPPLET_RELEASE_DIR)

###################################
# Red hat 
###################################

pkg-rpm: pre-pkg 
	make -C build-pkg build-rpm _VERSION=$(VERSION) _RELEASE=$(RELEASE)


pkg-rpm-release: pkg-rpm pkg-sf-cli-release
	cp build-pkg/install-native/*.rpm $(TAPPLET_RELEASE_DIR)


pkg-release: pkg-$(PKG)-release

