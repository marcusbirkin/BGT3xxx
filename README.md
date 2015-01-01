BGT3xxx
=======

Blackgold 3xxx Linux Drivers

As Blackgold seem to have abandoned development of the Linux drivers
this is an attempt to keep the offical blackgold drivers updated with new kernel versions.

DKMS INSTALL
============
cd /usr/src
sudo git clone -b dkms https://github.com/marcusbirkin/BGT3xxx.git
sudo cp BGT3xxx/bgt-linux-pcie-fw/dvb-fe-tda10048-1.0.fw /lib/firmware/
sudo ln -s  BGT3xxx/bgt-linux-pcie-drv/ BGT3xxx-1.0.0.0Fixed
sudo dkms add -m BGT3xxx -v 1.0.0.0Fixed
sudo dkms build -m BGT3xxx -v 1.0.0.0Fixed
sudo dkms install -m BGT3xxx -v 1.0.0.0Fixed
sudo modprobe saa7231_core
