#!/bin/bash


VPP_SRC_DIR=$1
INSTALL_PKG_DIR=$2

vpp_deb_dir=${VPP_SRC_DIR}/build-root


#### build ####
/bin/bash ${VPP_SRC_DIR}/build-root/vagrant/build.sh

make -C ${VPP_SRC_DIR} install-ext-deps

#### install dev ####

sudo dpkg -i ${VPP_SRC_DIR}/build-root/vpp-dev_*.deb
sudo dpkg -i ${VPP_SRC_DIR}/build/external/vpp-ext-deps_*.deb

#### collect pkgs ####

vpp_main_pkg=$(find ${vpp_deb_dir} -name vpp_*.deb)
vpp_lib_pkg=$(find ${vpp_deb_dir} -name vpp-lib*.deb)
vpp_dbg_pkg=$(find ${vpp_deb_dir} -name vpp-dbg*.deb)


cp ${vpp_main_pkg}  ${INSTALL_PKG_DIR}
cp ${vpp_lib_pkg}  ${INSTALL_PKG_DIR}
cp ${vpp_dbg_pkg}  ${INSTALL_PKG_DIR}