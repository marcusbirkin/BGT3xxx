#!/bin/bash

#Pull latest code
git pull
git reset --hard
git log -1

#Set execute flag on perl and bash scripts
chmod u+x bgt-linux-pcie-drv/v4l/scripts/*.pl
chmod u+x bgt-linux-pcie-drv/v4l/scripts/*.sh

#Change v4l and make
cd bgt-linux-pcie-drv/v4l
make clean
make -j4 KERNELRELEASE=$(uname -r) kernver=$(uname -r)
#Check for clean exit
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

#Install modules (requires root)
sudo make install
