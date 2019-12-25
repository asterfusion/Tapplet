
define prepare_directories
#prepare directories
	@rm -rf ./build-native 
	@rm -rf ./install-native
	@mkdir -p ./build-native
	@mkdir -p ./build-native/sftools
	@mkdir -p ./build-native/sfrest
	@mkdir -p ./build-native/nshell-lib
	@mkdir -p ./build-native/sf_plugins
	@mkdir -p ./build-native/sf_plugins_release
	@mkdir -p ./build-native/sf_plugins_debug
	@mkdir -p ./build-native/dpdk-setup
	@mkdir -p ./install-native
#copy files
	@cp  ../build/lib/vpp_plugins/*.so ./build-native/sf_plugins/
	@cp  ../build/lib/libnshell.so ./build-native/nshell-lib/
	@cp  ../build/bin/sf_*_shm ./build-native/sftools/
#copy dpdk-setup
	@cp  ./dpdk-setup/dpdk-devbind.py ./build-native/sftools/
endef


all:
	@echo "go to topdir and run "make" there"

###################################
# Debian 
###################################
.PHONY: build-deb install-deb clean-deb

build-deb:
	#prepare_directories
	$(call prepare_directories)
	@rm -rf ./deb/debian
	@cp -r ./dpdk-setup/vm/* ./build-native/dpdk-setup/
	@cp -r ./deb/debian-template ./deb/debian
	@cp -r ./sfrest/sfrest.service ./deb/debian/tapplet.sfrest.service
	@cp -r ./dpdk-setup/vpp.service.old ./build-native/dpdk-setup/
	#prepare deb
	@./scripts/generate-deb-changelog > ./deb/debian/changelog
	@./scripts/gen-tapplet-deb-install ./build-native/sf_plugins> ./deb/debian/tapplet.install
	#generate deb
	@cd ./deb &&  dpkg-buildpackage -us -uc -b > build.log 2>&1
	@grep -nr "warning" ./deb/build.log
	#mv files to  install dir
	@mv *.deb *.changes ./install-native

install-deb:
	dpkg -i ./install-native/*.deb

clean-deb:
	rm -rf ./deb/debian 
	rm -rfv ./deb/*.log

###################################
# Red hat 
###################################
.PHONY: build-rpm install-rpm clean-rpm

build-rpm:
	#prepare_directories
	$(call prepare_directories)
	#generate rpm
	make -C rpm _VERSION=$(_VERSION) _RELEASE=$(_RELEASE)
	#mv files to install dir
	@mv rpm/*.rpm ./install-native 

install-rpm:
	rpm -ivh ./install-native/*.rpm

clean-rpm:
	rm -rf ./rpm/rpmbuild


###################################################################

.PHONY: clean dist-clean
clean: 
	rm -rf ./build-native
	rm -rf ./install-native

dist-clean: clean-deb clean-rpm
	rm -rf ./build-native
	rm -rf ./install-native

