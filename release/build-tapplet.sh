#!/bin/bash


VPP_SRC_DIR=$1
TAPPLET_SRC_DIR=$2
INSTALL_PKG_DIR=$3

### build  tapplet###


export VPP_SRC_DIR=${VPP_SRC_DIR}

make -C ${TAPPLET_SRC_DIR} install-dep

make -C ${TAPPLET_SRC_DIR} build-release || exit 1

make -C ${TAPPLET_SRC_DIR} pkg-release


### collect tapplet ###
tapplet_deb_dir=${TAPPLET_SRC_DIR}/build-pkg/install-native/

tapplet_main_pkg=$(find ${tapplet_deb_dir} -name tapplet_*.deb)
tapplet_dbg_pkg=$(find ${tapplet_deb_dir} -name tapplet-dbg*.deb)


cp ${tapplet_main_pkg}  ${tapplet_dbg_pkg} ${INSTALL_PKG_DIR}
