import os

env = Environment()
env.Append(CCFLAGS = ["-std=c++0x", "-msse3", "-O3", "-Wall"])
env.Append(LIBS = [
	"SDL2",
	"png",
	"jpeg",
	"GL",
	"GLEW",
	"openal",
	"pthread"
]);
# Work with clang's static analyzer:
env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]
env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

opts = Variables()
opts.Add(PathVariable("PREFIX", "Directory to install under", "/usr/local", PathVariable.PathIsDirCreate))
opts.Update(env)

Help(opts.GenerateHelpText(env))

VariantDir("build", ".", duplicate = 0)

sky = env.Program("endless-sky", Glob("build/*.cpp"))


# Install the binary:
env.Install("$PREFIX/games", sky)

# Install the desktop file:
env.Install("$PREFIX/share/applications", "endless-sky.desktop")

# Install icons, keeping track of all the paths.
# Most Ubuntu apps supply 16, 22, 24, 32, 48, and 256, and sometimes others.
sizes = ["16x16", "22x22", "24x24", "32x32", "48x48", "256x256"]
icons = []
for size in sizes:
	destination = "$PREFIX/share/icons/hicolor/" + size + "/apps/endless-sky.png"
	icons.append(destination)
	env.InstallAs(destination, "endless-sky.iconset/icon_" + size + ".png")

# If any of those icons changed, also update the cache.
# Do not update the cache if we're not installing into "usr".
# (For example, this "install" may actually be creating a Debian package.)
if env.get("PREFIX").startswith("/usr/"):
	env.Command(
		[],
		icons,
		"gtk-update-icon-cache -t $PREFIX/share/icons/hicolor/")

# Install the man page.
env.Command(
	"$PREFIX/share/man/man6/endless-sky.6.gz",
	"endless-sky.6",
	"gzip -c $SOURCE > $TARGET")

# Install the data files.
def RecursiveInstall(env, target, source):
	rootIndex = len(env.Dir(source).abspath) + 1
	for node in env.Glob(os.path.join(source, '*')):
		if node.isdir():
			name = node.abspath[rootIndex:]
			RecursiveInstall(env, os.path.join(target, name), node.abspath)
		else:
			env.Install(target, node)
RecursiveInstall(env, "$PREFIX/share/games/endless-sky/data", "data")
RecursiveInstall(env, "$PREFIX/share/games/endless-sky/images", "images")
RecursiveInstall(env, "$PREFIX/share/games/endless-sky/sounds", "sounds")
env.Install("$PREFIX/share/games/endless-sky", "credits.txt")
env.Install("$PREFIX/share/games/endless-sky", "keys.txt")

# Make the word "install" in the command line do an installation.
env.Alias("install", "$PREFIX")
