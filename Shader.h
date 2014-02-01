/* Shader.h
*/

#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <GL/glew.h>



class Shader {
public:
	Shader() = default;
	Shader(const char *vertex, const char *fragment);
	
	GLuint Object() const;
	GLint Attrib(const char *name) const;
	GLint Uniform(const char *name) const;
	
	
private:
	GLuint Compile(const char *str, GLenum type);
	
	
private:
	GLuint program;
};



#endif
