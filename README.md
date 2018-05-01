# EyeRecToo

EyeRecToo is a second-generation hardware-agnostic open-source software for
head-mounted eye trackers.  Its main raison d'être is to provide an open
platform to replace the data acquisition functionality from eye-tracker
vendors' software, which typically are expensive, closed-source, and geared
toward their own devices.
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
>This is the platform that gets the most testing and recommended.

- Initiall support for Ubuntu 16.04 64 bits has been added; no binaries available at the moment though.
>Only supports camera access using the uvcengine, including the Pupil Labs eye tracker. It has been tested for
pupil detection, gaze estimation, and data recording. This is **alpha**.

- Notes

>It's also possible to get EyeRecToo running on 32 bits platforms, but no Pupil
>eye tracker support is available.

## Supported Devices

Theoretically, cameras using DirectShow (on Windows) and v4l2 (on Linux)
should work out of the box.

UVC-compliant cameras are suuported through the [UVC Engine](https://atreus.informatik.uni-tuebingen.de/santini/uvcengine).
On Windows, it is typically necessary to change camera drivers to be able to
use the uvcengine; see the **Running** section for details.

*Tested Eye Trackers:*
- Dikablis Essential / Pro (see [Ergoneers](http://www.ergoneers.com))
- Pupil DIY (see [Pupil Labs](https://pupil-labs.com/))
- Eivazi's microscope add-on (see [Eivazi, S et al.  2016](http://ieeexplore.ieee.org/document/7329925/))
- Pupil Eye Tracker, both Cam1 and Cam2 models (see [Pupil Labs](https://pupil-labs.com/store/))
- The inconspicuous modular eye trackers from [Eivazi, S et al 2018](http://www.ti.uni-tuebingen.de/typo3conf/ext/timitarbeiter/pi4/show_bibtex.php?uid=887&tid=1)

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

**NOTE:** Additionally, you may need to install drivers to access your eye tracker
cameras:

1. Dikablis eye trackers require the
[VRMagic drivers](https://www.vrmagic.com/imaging/downloads/) (make sure you
have the 64 bits version)
2. For Pupil eye trackers, check the [Pupil Labs' guide](https://github.com/pupil-labs/pupil/wiki/Windows-Driver-Setup).
3. (Windows only) for better interfacing with UVC-compliant cameras, we recommend to change the camera drivers to enable the uvcengine.
This can be easily achieved through [Zadig](https://zadig.akeo.ie/):
a) Check *Options->"list all devices"*,
b) uncheck *Options->"Ignore Hubs or Composite Parents"* ,
c) change your camera's *interface* driver to *libusbK*,
d) change your camera's *composite parent* to *libusbK*

## Developing

The official EyeRecToo repository is located at:
[https://atreus.informatik.uni-tuebingen.de/santini/EyeRecToo](https://atreus.informatik.uni-tuebingen.de/santini/EyeRecToo)

**Windows**

The particulars of the test build system we use are:
- QtCreator 4.0.3
- Qt >= 5.7.0 (MSVC 2015, 64 bit)
- Visual Studio 2015
- Microsoft Windows 8.1

All libraries/includes (for Windows 64 bits) should be in the deps directory already; to build:
1. Install Visual Studio 2015
2. Install Qt 5.7.0 (and Qt Creator)
3. Open the project
4. Run qmake and build
5. **Before running, add deps/runtime/Release or deps/runtime/Debug to your PATH accordingly to your build type.**

*Both Release and Debug versions are functional although the latter may run at much lower sampling rates.*

**Linux**

The particulars of the test build system we use are:
- QtCreator 4.4.1
- Qt 5.9.2
- GCC 5.4.0
- Ubuntu 16.04.3

Requires the installation of some packages (see README.LINUX); to build:
1. Install Qt 5.9.2 (and Qt Creator)
2. Run ./linux-setup.sh (or do the steps manually)
3. Open the project
4. Run qmake and build
5. Should be good to go :-)

