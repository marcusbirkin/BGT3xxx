BGT3xxx
=======

Blackgold 3xxx Linux Drivers

As Blackgold seem to have abandoned development of the Linux drivers this is an attempt to keep the official Blackgold drivers updated with new kernel versions.

Last updated for kernels up to v4.10
Newer kernel support is available here: http://git.cblinux.co.uk:5010/Blackgold/BGT3xxx-fork-for-cxd2837 

*n.b. I no longer operate Blackgold hardware due to hardware quality issues and lack of manufacturer support*

STANDARD INSTALL
================
cd /usr/src  
sudo git clone https://github.com/marcusbirkin/BGT3xxx.git  
cd BGT3xxx
sudo ./build.sh  
sudo reboot

DKMS INSTALL (Auto recompile on kernel update)
==============================================
cd /usr/src  
sudo git clone https://github.com/marcusbirkin/BGT3xxx.git  
sudo cp BGT3xxx/bgt-linux-pcie-fw/dvb-fe-tda10048-1.0.fw /lib/firmware/  
sudo ln -s  BGT3xxx/bgt-linux-pcie-drv/ BGT3xxx-1.0.0.0Fixed  
sudo dkms add -m BGT3xxx -v 1.0.0.0Fixed  
sudo dkms build -m BGT3xxx -v 1.0.0.0Fixed  
sudo dkms install -m BGT3xxx -v 1.0.0.0Fixed  
sudo reboot
