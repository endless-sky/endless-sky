libpng
======

*libpng.a* is v1.6.25, from
[here](https://sourceforge.net/projects/libpng/files/).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cd libpng-1.6.25
cp scripts/makefile.darwin .
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Edit *makefile.darwin*: append `-mmacosx-version-min=10.7` to the `CFLAGS` and
`CPPFLAGS` lines in order to avoid some annoying warnings when later compiling
EndlessSky in Xcode.

Edit *pnglibconf.h*: comment out the `#define PNG_WARNINGS_SUPPORTED` line to
avoid libpng warnings from EndlessSky in the log.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
make -f makefile.darwin libpng.a
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This builds *libpng.a*, and headers (from the *include* directory) were also
copied here.

libturbojpeg
============

*libturbojpeg.a* is v1.5.0, built using [brew](http://brew.sh).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
brew install libjpeg-turbo
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This builds *libturbojpeg.a* at */usr/local/Cellar/jpeg-turbo/1.5.0/lib*

SDL2
====

*SDL2.framework* was obtained as described in the readme-developer:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
hg clone https://hg.libsdl.org/SDL; open SDL/Xcode/SDL/SDL.xcodeproj
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In Xcode, select the *Project* \> *Archive* menu. *SDL2.framework* will be at
*SDL/Build/Intermediates/ArchiveIntermediates/Framework/InstallationBuildProductsLocation\@rpath*
