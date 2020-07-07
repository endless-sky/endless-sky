# steamrt scons is v2.1.0
# https://repo.steampowered.com/steamrt-images-scout/snapshots/latest-container-runtime-depot/com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sources.sources.txt
# Documentation available at https://scons.org/doc/2.1.0/HTML/scons-user/a10706.html
import os
import platform
from SCons.Node.FS import Dir

def pathjoin(*args):
	return os.path.join(*args)

# Load environment variables, including some that should be renamed.
# If we are compiling on Windows, then we need to change the toolset to MinGW.
is_windows_host = platform.system().startswith('Windows')
scons_toolset = ['mingw' if is_windows_host else 'default']
env = DefaultEnvironment(tools = scons_toolset, ENV = os.environ)

if 'CXX' in os.environ:
	env['CXX'] = os.environ['CXX']
if 'CXXFLAGS' in os.environ:
	env.Append(CCFLAGS = os.environ['CXXFLAGS'])
if 'LDFLAGS' in os.environ:
	env.Append(LINKFLAGS = os.environ['LDFLAGS'])
if 'AR' in os.environ:
	env['AR'] = os.environ['AR']
if 'DIR_ESLIB' in os.environ:
	path = os.environ['DIR_ESLIB']
	env.Prepend(CPPPATH = [pathjoin(path, 'include')])
	env.Append(LIBPATH = [pathjoin(path, 'lib')])

# Don't spawn a console window by default on Windows builds.
if is_windows_host:
	env.Append(LINKFLAGS = ["-mwindows"])

opts = Variables()
opts.AddVariables(
	EnumVariable("mode", "Compilation mode", "release", allowed_values=("release", "debug", "profile")),
	EnumVariable("opengl", "Whether to use OpenGL or OpenGL ES", "desktop", allowed_values=("desktop", "gles")),
	PathVariable("BUILDDIR", "Directory to store compiled object files in", "build", PathVariable.PathIsDirCreate),
	PathVariable("BIN_DIR", "Directory to store binaries in", ".", PathVariable.PathIsDirCreate),
	PathVariable("DESTDIR", "Destination root directory, e.g. if building a package", "", PathVariable.PathAccept),
	PathVariable("PREFIX", "Directory to install under (will be prefixed by DESTDIR)", "/usr/local", PathVariable.PathIsDirCreate),
)
opts.Update(env)
Help(opts.GenerateHelpText(env))

# Required build flags. To enable SSE or other optimizations, pass CXXFLAGS via the environment
#   $ CXXFLAGS=-msse3 scons
#   $ CXXFLAGS=-march=native scons
# or modify the `flags` variable:
flags = ["-std=c++11", "-Wall", "-Werror", "-Wold-style-cast"]
if env["mode"] != "debug":
	flags += ["-O3", "-flto"]
	env.Append(LINKFLAGS = ["-O3", "-flto"])
if env["mode"] == "debug":
	flags += ["-g"]
elif env["mode"] == "profile":
	flags += ["-pg"]
	env.Append(LINKFLAGS = ["-pg"])
env.Append(CCFLAGS = flags)

# Always use `ar` to create the symbol table, and don't use ranlib at all, since it fails to preserve
# LTO information, even when passed the plugin path, when run in Steam's "Scout" runtime.
env['RANLIBCOM'] = ''
# TODO: can we derive thin archive support from the host system somehow? Or just always use it?
create_thin_archives = env.get('AR', '').startswith('gcc')
env.Replace(ARFLAGS = 'rcsT' if create_thin_archives else 'rcs')

# The Steam runtime fails to correctly invoke the LTO plugin for gcc-5, so pass it explicitly
chroot_name = os.environ.get('SCHROOT_CHROOT_NAME', '')
if 'steamrt_scout' in chroot_name:
	# MAYBE: read g++ version to determine correct path to the LTO plugin
	plugin_path = '--plugin={}'.format(os.environ.get('LTO_PLUGIN_PATH', '/usr/lib/gcc/x86_64-linux-gnu/5/liblto_plugin.so'))
	env.Append(ARFLAGS = [plugin_path])

# Required system libraries, such as UUID generator runtimes.
sys_libs = [
	"rpcrt4",
] if is_windows_host else [
	"uuid"
]
env.Append(LIBS = sys_libs)

game_libs = [
	"winmm",
	"mingw32",
	"sdl2main",
	"sdl2.dll",
	"png.dll",
	"turbojpeg.dll",
	"jpeg.dll",
	"openal32.dll",
	"glew32.dll",
	"opengl32",
] if is_windows_host else [
	"SDL2",
	"png",
	"jpeg",
	"openal",
	"pthread",
]
env.Append(LIBS = game_libs)

if env["opengl"] == "desktop":
	env.Append(LIBS = [
		"GL",
		"GLEW"
	]);
else:
	env.Append(LIBS = [
		"GLESv2"
	]);
	flags += ["-DES_GLES"]


# Required build flags. If you want to use SSE optimization, you can turn on
# -msse3 or (if just building for your own computer) -march=native.
env.Append(CCFLAGS = flags)

# libmad is not in the Steam runtime, so link it statically:
if 'steamrt_scout_i386' in chroot_name:
	env.Append(LIBS = File("/usr/lib/i386-linux-gnu/libmad.a"))
elif 'steamrt_scout_amd64' in chroot_name:
	env.Append(LIBS = File("/usr/lib/x86_64-linux-gnu/libmad.a"))
else:
	env.Append(LIBS = "mad")


binDirectory = '' if env["BIN_DIR"] == '.' else pathjoin(env["BIN_DIR"], env["mode"])
buildDirectory = pathjoin(env["BUILDDIR"], env["mode"])
libDirectory = pathjoin("lib", env["mode"])
VariantDir(buildDirectory, "source", duplicate = 0)

# Find all regular source files.
def RecursiveGlob(pattern, dir_name=buildDirectory):
	# Start with source files in subdirectories.
	matches = [RecursiveGlob(pattern, sub_dir) for sub_dir in Glob(pathjoin(str(dir_name), "*"))
		if isinstance(sub_dir, Dir)]
	# Add source files in this directory, except for main.cpp
	matches += Glob(pathjoin(str(dir_name), pattern))
	matches = [i for i in matches if not "/main.cpp" in str(i)]
	return matches

# By default, invoking scons will build the backing archive file and then the game binary.
sourceLib = env.StaticLibrary(pathjoin(libDirectory, "endless-sky"), RecursiveGlob("*.cpp", buildDirectory))
exeObjs = [Glob(pathjoin(buildDirectory, f)) for f in ("main.cpp",)]
if is_windows_host:
	windows_icon = env.RES(pathjoin(buildDirectory, "WinApp.rc"))
	exeObjs.append(windows_icon)
sky = env.Program(pathjoin(binDirectory, "endless-sky"), exeObjs + sourceLib)
env.Default(sky)


# The testing infrastructure ignores "mode" specification (i.e. we only test optimized output).
# (If we add support for code coverage output, this will likely need to change.)
testBuildDirectory = pathjoin("tests", env["BUILDDIR"])
VariantDir(testBuildDirectory, pathjoin("tests", "src"), duplicate = 0)
test = env.Program(
	target=pathjoin("tests", "endless-sky-tests"),
	source=RecursiveGlob("*.cpp", testBuildDirectory) + sourceLib,
	 # Add Catch header & additional test includes to the existing search paths
	CPPPATH=(env.get('CPPPATH', []) + [pathjoin('tests', 'include')]),
	# Do not link against the actual implementations of SDL, OpenGL, etc.
	LIBS=sys_libs,
	# Pass the necessary link flags for a console program.
	LINKFLAGS=[x for x in env.get('LINKFLAGS', []) if x not in ('-mwindows',)]
)
# Invoking scons with the `build-tests` target will build the unit test framework
env.Alias("build-tests", test)
# Invoking scons with the `test` target will build (if necessary) and
# execute the unit test framework (always). All non-hidden tests are run.
catch2_args = " " + " ".join([
	"-i",
	"--warn NoAssertions",
	"--order rand",
	"--rng-seed 'time'",
])
test_runner = env.Action(test[0].abspath + catch2_args, 'Running tests...')
env.Alias("test", test, test_runner)
env.AlwaysBuild("test")


# Install the binary:
env.Install("$DESTDIR$PREFIX/games", sky)

# Install the desktop file:
env.Install("$DESTDIR$PREFIX/share/applications", "endless-sky.desktop")

# Install app center metadata:
env.Install("$DESTDIR$PREFIX/share/appdata", "endless-sky.appdata.xml")

# Install icons, keeping track of all the paths.
# Most Ubuntu apps supply 16, 22, 24, 32, 48, and 256, and sometimes others.
sizes = ["16x16", "22x22", "24x24", "32x32", "48x48", "128x128", "256x256", "512x512"]
icons = []
for size in sizes:
	destination = "$DESTDIR$PREFIX/share/icons/hicolor/" + size + "/apps/endless-sky.png"
	icons.append(destination)
	env.InstallAs(destination, "icons/icon_" + size + ".png")

# If any of those icons changed, also update the cache.
# Do not update the cache if we're not installing into "usr".
# (For example, this "install" may actually be creating a Debian package.)
if env.get("PREFIX").startswith("/usr/"):
	env.Command(
		[],
		icons,
		"gtk-update-icon-cache -t $DESTDIR$PREFIX/share/icons/hicolor/")

# Install the man page.
env.Command(
	"$DESTDIR$PREFIX/share/man/man6/endless-sky.6.gz",
	"endless-sky.6",
	"gzip -c $SOURCE > $TARGET")

# Install the data files.
def RecursiveInstall(env, target, source):
	rootIndex = len(env.Dir(source).abspath) + 1
	for node in env.Glob(pathjoin(source, '*')):
		if node.isdir():
			name = node.abspath[rootIndex:]
			RecursiveInstall(env, pathjoin(target, name), node.abspath)
		else:
			env.Install(target, node)
RecursiveInstall(env, "$DESTDIR$PREFIX/share/games/endless-sky/data", "data")
RecursiveInstall(env, "$DESTDIR$PREFIX/share/games/endless-sky/images", "images")
RecursiveInstall(env, "$DESTDIR$PREFIX/share/games/endless-sky/sounds", "sounds")
env.Install("$DESTDIR$PREFIX/share/games/endless-sky", "credits.txt")
env.Install("$DESTDIR$PREFIX/share/games/endless-sky", "keys.txt")

# Make the word "install" in the command line do an installation.
env.Alias("install", "$DESTDIR$PREFIX")
