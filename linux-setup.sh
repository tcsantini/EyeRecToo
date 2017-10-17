#!/bin/bash

# Change this to point to your Qt directory
QTDIR=/media/data/Qt

sudo apt-get install build-essential
sudo apt-get install libgl1-mesa-dev
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
sudo apt-get install libusb-1.0-0
sudo apt-get install libturbojpeg
sudo ln -s /usr/lib/x86_64-linux-gnu/libturbojpeg.so.0.1.0 /usr/lib/x86_64-linux-gnu/libturbojpeg.so
sudo cp EyeRecToo/deps/udev-rules/10-libuvc.rules /etc/udev/rules.d/
mv ${QTDIR}/5.9.2/gcc_64/plugins/mediaservice/libgstcamerabin.so ~/libgstcamerabin.so
if [ $? != 0 ]
then
	echo "WARNING: Could not move libgstcamerabin.so"
	echo "Is Qt installed at ${QTDIR}?"
fi
