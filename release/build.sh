#!/bin/bash

TAPPLET_SRC_DIR=$(pwd)

EXTERNAL_BUILD_DIR=$TAPPLET_SRC_DIR/release/build/external/
VPP_SRC_DIR=$EXTERNAL_BUILD_DIR/vpp1810
INSTALL_PKG_DIR=$TAPPLET_SRC_DIR/release/build/install-native

export LC_ALL='en_US.UTF-8'
rm -rf $INSTALL_PKG_DIR
mkdir -p $INSTALL_PKG_DIR

### os info ###

OS_ID=$(grep '^ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
OS_VERSION_ID=$(grep '^VERSION_ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')

KERNEL_OS=`uname -o`
KERNEL_MACHINE=`uname -m`
KERNEL_RELEASE=`uname -r`
KERNEL_VERSION=`uname -v`

echo KERNEL_OS: $KERNEL_OS
echo KERNEL_MACHINE: $KERNEL_MACHINE
echo KERNEL_RELEASE: $KERNEL_RELEASE
echo KERNEL_VERSION: $KERNEL_VERSION
echo OS_ID: $OS_ID
echo OS_VERSION_ID: $OS_ID

echo TAPPLET_SRC_DIR: $TAPPLET_SRC_DIR

### download vpp ###
echo "======  download vpp ======"
if [ -d $VPP_SRC_DIR ]
then 
    echo "-- download finsh --"
else

    if [ -d $EXTERNAL_BUILD_DIR ]
    then
        rm -rf $EXTERNAL_BUILD_DIR 
    fi
    mkdir -p $EXTERNAL_BUILD_DIR

    cd $EXTERNAL_BUILD_DIR ; git clone -b stable/1810  https://github.com/FDio/vpp.git
    cp -r $EXTERNAL_BUILD_DIR/vpp $VPP_SRC_DIR
fi

### patch vpp ###
echo "======  patch vpp ======"
patch -p1 -d ${VPP_SRC_DIR} < $TAPPLET_SRC_DIR/release/patchs/vpp1810.patch
echo "-- patch finsh --"

### build vpp  , install vpp-dev && dpdk-dev ###

if [ "$OS_ID" == "ubuntu" ]; then
    /bin/bash ${TAPPLET_SRC_DIR}/release/deb/build_vpp.sh ${VPP_SRC_DIR} ${INSTALL_PKG_DIR}
elif [ "$OS_ID" == "debian" ]; then
    /bin/bash ${TAPPLET_SRC_DIR}/release/deb/build_vpp.sh ${VPP_SRC_DIR} ${INSTALL_PKG_DIR}
elif [ "$OS_ID" == "centos" ]; then
    echo "[Error] Not surpported now"
    exit 1
else
    echo "[Error] Not surpported now"
    exit 1
fi

### build tapplet ###

/bin/bash ${TAPPLET_SRC_DIR}/release/build-tapplet.sh ${VPP_SRC_DIR} ${TAPPLET_SRC_DIR} ${INSTALL_PKG_DIR}
