# EyeRecToo

EyeRecToo is a second-generation open-source software for head-mounted eye trackers.
Its main raison d'être is to provide an open platform to replace the data
acquisition functionality from eye-tracker vendors' software, which typically
are expensive, closed-source, and geared toward their own devices.
It deprecates [EyeRec](https://www-ti.informatik.uni-tuebingen.de/santini/EyeRec).

For data analysis functionality replacement, we recommend
[Eyetrace](http://www.ti.uni-tuebingen.de/Eyetrace.1751.0.html).

>*Why the name?*
>
>Originally, EyeRec was a phonetical play on words and the
>eye-related functionality as in **I rec**[ord].
>EyeRecToo adds on the word play: It is the version **two** and, similar to EyeRec, records **too**.
>All bad puns are intended :-)

## Supported Platforms

- Windows 64 bits

>Nonetheless, it also has been previously ran on Ubuntu 16.04 (64 bits) and a
>couple other Linux distributions.
>It's also possible to get EyeRecToo running on 32 bits platforms, but no Pupil
>eye tracker supported is available.

## Supported Devices

Theoretically, cameras using DirectShow (on Windows) and v4l2 (on Linux)
should work out of the box.

*Tested Eye Trackers:*
- Dikablis Essential / Pro (see [Ergoneers](http://www.ergoneers.com))
- Pupil DIY (see [Pupil Labs](https://pupil-labs.com/))
- Eivazi's microscope add-on (see [Eivazi, S et al.  2016](http://ieeexplore.ieee.org/document/7329925/))
- Pupil Eye Tracker (see [Pupil Labs](https://pupil-labs.com/store/))
**NOTE:** Early support for Pupil Labs' custom cameras is provided
through the [UVC Engine](https://atreus.informatik.uni-tuebingen.de/santini/uvcengine).
This support was tested with a binocular Pupil headset (in total three cameras) with the
following configurations:
1. Eyes (320 x 240 @120fps) Field (1280 x 720 @30fps)
2. Eyes (640 x 480 @120fps) Field (1280 x 720 @30fps)
3. Eyes (640 x 480 @120fps) Field (640 x 480 @120fps)

Other configurations also work but were not extensively used.

*Please note that the camera driver, libusb, and libuvc can crash if the headset is unplugged
while the cameras are streaming. These are still work in progress for Windows,
and the uvc engine is still in beta.*


*Tested webcams:*
- [Playstation Eye](https://en.wikipedia.org/wiki/PlayStation_Eye)
- [Microsoft LifeCam HD-3000](https://www.microsoft.com/accessories/en-us/products/webcams/lifecam-hd-3000/t3h-00011)
- [Microsoft LifeCam HD-5000](https://www.microsoft.com/accessories/en-us/d/lifecam-hd-5000)
- [Microsoft LifeCam HD-6000](https://www.microsoft.com/accessories/en-us/d/lifecam-hd-6000-for-notebooks)
- [Microsoft LifeCam VX-1000](https://www.microsoft.com/accessories/en-us/d/lifecam-vx-1000)
- [Logitech C920](http://www.logitech.com/en-us/product/hd-pro-webcam-c920)
- [Logitech C510](http://support.logitech.com/en_us/product/hd-webcam-c510)
- [Logitech C525](http://www.logitech.com/en-us/product/hd-webcam-c525)
- [Logitech QuickCam PRO 9000](http://support.logitech.com/en_us/product/quickcam-pro-9000)
- [Logitech QuickCam Express](http://support.logitech.com/en_us/product/quickcam-express)


## Running

Binaries can be downloaded [here](www.ti.uni-tuebingen.de/perception).

Prior to running, make sure you have the Visual C++ 2015 x64 run-time
components installed. If not, you can install them by running
vcredist_x64.exe or
[downloading the installer directly from Microsoft](https://download.microsoft.com/download/9/3/F/93FCF1E7-E6A4-478B-96E7-D4B285925B00/vc_redist.x64.exe).

**NOTE:** Additionally, you may need to install drivers to access your eye tracker
cameras:

1. Dikablis eye trackers require the
[VRMagic drivers](https://www.vrmagic.com/imaging/downloads/) (make sure you
have the 64 bits version)
2. For Pupil eye trackers, check the [Pupil Labs' guide](https://github.com/pupil-labs/pupil/wiki/Windows-Driver-Setup).

## Developing

The EyeRecToo official repository is located at:
[https://atreus.informatik.uni-tuebingen.de/santini/EyeRecToo](https://atreus.informatik.uni-tuebingen.de/santini/EyeRecToo)

The particulars of the test build system we use are:
- QtCreator 4.0.3
- Qt 5.7.0 (MSVC 2015, 64 bit)
- Visual Studio 2015
- Microsoft Windows 8.1

All libraries/includes (for Windows 64 bits) should be in the deps directory already; to build:
1. Install Visual Studio 2015
2. Install Qt 5.7.0 (and Qt Creator)
3. Open the project
4. Run qmake and build
5. Before running, add deps/runtime/Release or deps/runtime/Debug to your PATH.

*Both Release and Debug versions are functional although the latter may run at a low sampling rates.*

