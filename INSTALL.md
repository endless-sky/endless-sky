######Building Endless Sky
_NOTE: If you have strayed here by accident and are inexperienced with building
software and the command line, it may be a better idea to grab the program from
your package manager, the releases section in GitHub, or Steam, rather than
fruitlessly wasting your time_

###Linux
You must install both headers and runtimes of the following libraries:
- sdl2
- libpng
- libjpeg-turbo
- glew
- openal
- mad

The following tools are necessary to proceed:
- C++ compiler that supports C++11 (GCC 4.8.1+, clang 3.3+)
- GNU make
- pkg-config

You then run the following commands in the same directory as the `Makefile`:

    $ make
    $ ./endless-sky

`-j9` may be passed to `make` to make the build go faster at the expense of
memory and CPU time. Setting an environment variable `OPTFLAGS` will alter the
default optimisation flags, while `CXXFLAGS` and `LDFLAGS` will let you change
the compiler and linker flags respectively, if any changes are necessary.

The program will infer the location of the `data` and `images` directories
according to the rules outlined in the man page `endless-sky.6`.

Debian-based distributions will probably want to install all necessary
dependencies using the following command, requiring root privileges:

    # apt install build-essential lib{sdl2,png12,jpeg62-turbo,glew,openal,mad0}-dev

###Mac OS X
You must install [Homebrew](https://brew.sh), then you must install the
following packages using `brew install`:

    $ brew install pkg-config sdl2 openal-soft jpeg-turbo lib{png,mad}

You must also either set `PKG_CONFIG_PATH` or follow the instructions according
to the output of homebrew after you install any of those packages in order for
`pkg-config` to be able to find its library definition files necessary to
proceed.

Example output:
    This formula is keg-only, which means it was not symlinked into /usr/local,
    because macOS provides OpenAL.framework.

    If you need to have this software first in your PATH run:
    echo 'export PATH="/usr/local/opt/openal-soft/bin:$PATH"' >> ~/.bash_profile

    For compilers to find this software you may need to set:
        LDFLAGS:  -L/usr/local/opt/openal-soft/lib
    	CPPFLAGS: -I/usr/local/opt/openal-soft/include
    For pkg-config to find this software you may need to set:
    	PKG_CONFIG_PATH: /usr/local/opt/openal-soft/lib/pkgconfig

Unfortunately, Mac OS X does not seem to provide any `gl.pc`. You can work
around this by either manually editing the Makefile to allow you to compile by
passing special CXXFLAGS and LDFLAGS environment variables, or you can use this
file, taken from [here](https://stackoverflow.com/questions/29457534/cmake-on-osx-yosemite-10-10-3-glew-package-gl-not-found):

    PACKAGE=GL

    Name: OpenGL
    Description: OpenGL
    Version: 11.1.1
    Cflags: -framework OpenGL -framework AGL
    Libs: -Wl,-framework,OpenGL,-framework,AGL

Simply create a plain text file named `gl.pc` with a plain text editor, place
the above contents with no leading whitespace inside said file, then place that
file in `/usr/lib/pkgconfig`.

After sufficient cursing at Apple, you may proceed to build and run:

    $ make
    $ ./endless-sky

###Windows
TODO
