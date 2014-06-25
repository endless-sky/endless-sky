env = Environment()
env.Append(CCFLAGS = ["-std=c++0x", "-msse4.1", "-03", "-Wall"])
env.Append(LIBS = [
	"SDL2",
	"SDL2_image",
	"GL",
	"GLEW"
]);

VariantDir("build", ".", duplicate = 0)

sky = env.Program("sky", Glob("build/*.cpp"))
