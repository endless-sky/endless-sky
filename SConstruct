env = Environment()
env.Append(CCFLAGS = ["-std=c++0x", "-msse4.1", "-O3", "-Wall"])
env.Append(LIBS = [
	"boost_filesystem-mt",
	"boost_system-mt",
	"SDL",
	"SDL_image",
	"GL",
	"GLEW"
]);

VariantDir("build", ".", duplicate = 0)

sky = env.Program("sky", Glob("build/*.cpp"))
