env = Environment()
env.Append(CCFLAGS = ["-std=c++0x", "-msse4.1", "-gdwarf-2", "-Wall"])
env.Append(LIBS = [
	"SDL2",
	"png",
	"jpeg",
	"GL",
	"GLEW"
]);

VariantDir("build", ".", duplicate = 0)

sky = env.Program("sky", Glob("build/*.cpp"))
