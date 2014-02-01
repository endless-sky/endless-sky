/* Shader.cpp
*/

#include "Shader.h"

#include <stdexcept>
#include <iostream>

using namespace std;



Shader::Shader(const char *vertex, const char *fragment)
{
	GLuint vertexShader = Compile(vertex, GL_VERTEX_SHADER);
	GLuint fragmentShader = Compile(fragment, GL_FRAGMENT_SHADER);
	
	program = glCreateProgram();
	if(!program)
		throw runtime_error("Creating OpenGL shader program failed.");
	
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	
	glLinkProgram(program);
	
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if(status == GL_FALSE)
		throw runtime_error("Linking OpenGL shader program failed.");
}



GLuint Shader::Object() const
{
	return program;
}



GLint Shader::Attrib(const char *name) const
{
	GLint attrib = glGetAttribLocation(program, name);
	if(attrib == -1)
		throw runtime_error("Attribute \"" + string(name) + "\" not found.");
	
	return attrib;
}



GLint Shader::Uniform(const char *name) const
{
	GLint uniform = glGetUniformLocation(program, name);
	if(uniform == -1)
		throw runtime_error("Uniform \"" + string(name) + "\" not found.");
	
	return uniform;
}



GLuint Shader::Compile(const char *str, GLenum type)
{
	GLuint object = glCreateShader(type);
	if(!object)
		throw runtime_error("Shader creation failed.");
	
	glShaderSource(object, 1, (const GLchar**)&str, NULL);
	glCompileShader(object);
	
	GLint status;
	glGetShaderiv(object, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE)
	{
		static const int SIZE = 4096;
		GLchar message[SIZE];
		GLsizei length;
		
		glGetShaderInfoLog(object, SIZE, &length, message);
		cerr.write(message, length);
		throw runtime_error("Shader compilation failed.");
	}
	
	return object;
}
