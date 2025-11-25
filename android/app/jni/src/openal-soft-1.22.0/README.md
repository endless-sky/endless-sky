OpenAL soft
===========

`master` branch CI status :  [![Build Status](https://travis-ci.org/kcat/openal-soft.svg?branch=master)](https://travis-ci.org/kcat/openal-soft) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/kcat/openal-soft?branch=master&svg=true)](https://ci.appveyor.com/api/projects/status/github/kcat/openal-soft?branch=master&svg=true)

OpenAL Soft is an LGPL-licensed, cross-platform, software implementation of the OpenAL 3D audio API. It's forked from the open-sourced Windows version available originally from openal.org's SVN repository (now defunct).
OpenAL provides capabilities for playing audio in a virtual 3D environment. Distance attenuation, doppler shift, and directional sound emitters are among the features handled by the API. More advanced effects, including air absorption, occlusion, and environmental reverb, are available through the EFX extension. It also facilitates streaming audio, multi-channel buffers, and audio capture.

More information is available on the [official website](http://openal-soft.org/)

Source Install
-------------
To install OpenAL Soft, use your favorite shell to go into the build/
directory, and run:

```bash
cmake ..
```

Assuming configuration went well, you can then build it, typically using GNU
Make (KDevelop, MSVC, and others are possible depending on your system setup
and CMake configuration).

Please Note: Double check that the appropriate backends were detected. Often,
complaints of no sound, crashing, and missing devices can be solved by making
sure the correct backends are being used. CMake's output will identify which
backends were enabled.

For most systems, you will likely want to make sure ALSA, OSS, and PulseAudio
were detected (if your target system uses them). For Windows, make sure
DirectSound was detected.


Utilities
---------
The source package comes with an informational utility, openal-info, and is
built by default. It prints out information provided by the ALC and AL sub-
systems, including discovered devices, version information, and extensions.


Configuration
-------------

OpenAL Soft can be configured on a per-user and per-system basis. This allows
users and sysadmins to control information provided to applications, as well
as application-agnostic behavior of the library. See alsoftrc.sample for
available settings.


Acknowledgements
----------------

Special thanks go to:

 - Creative Labs for the original source code this is based off of.
 - Christopher Fitzgerald for the current reverb effect implementation, and
helping with the low-pass and HRTF filters.
 - Christian Borss for the 3D panning code previous versions used as a base.
 - Ben Davis for the idea behind a previous version of the click-removal code.
 - Richard Furse for helping with my understanding of Ambisonics that is used by
the various parts of the library.
