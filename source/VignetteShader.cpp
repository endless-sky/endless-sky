/* VignetteShader.cpp
Copyright (c) 2023 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "VignetteShader.h"

#include "Screen.h"
#include "Shader.h"

#include "opengl.h"

using namespace std;

namespace {
	// OpenGL objects:
	Shader shader;
	GLuint fogI;
	GLuint zoomI;
	GLuint dimensionsI;

	GLuint vao;
}



void VignetteShader::Init()
{
	static const char *vertexCode = R"(
// vertex vignette shader
const vec2 list[] = vec2[](
  vec2(-1., -1.),
  vec2(-1., +3.),
  vec2(+3., -1.)
);

void main() {
  gl_Position = vec4(list[gl_VertexID], 0, 1);
}
	)";

	static const char *fragmentCode = R"(
// fragment vignette shader
precision mediump float;
uniform float fog;
uniform float zoom;
uniform vec2 dimensions;

out vec4 finalColor;

void main() {
  vec2 uv = gl_FragCoord.xy / dimensions;
  uv = (uv - .5) * fog / (500 * zoom) + .5;

  if(uv.x < 0. || uv.x > 1. || uv.y < 0. || uv.y > 1.)
    finalColor = vec4(0., 0., 0., 1.);
  else
  {
    uv *= 1. - uv.yx;
    finalColor = vec4(0., 0., 0., 1. - pow(uv.x * uv.y * 18., 3.5));
  }
}
	)";

	// Compile the shader and store indices to its variables.
	shader = Shader(vertexCode, fragmentCode);
	fogI = shader.Uniform("fog");
	zoomI = shader.Uniform("zoom");
	dimensionsI = shader.Uniform("dimensions");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindVertexArray(0);
}



void VignetteShader::Draw(double fog, double zoom)
{
	if(fog <= 0.)
		return;

	// Set up to draw the vignette.
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	glUniform1f(fogI, static_cast<GLfloat>(fog));
	glUniform1f(zoomI, static_cast<GLfloat>(zoom));
	GLfloat dimensions[2] = {
		static_cast<GLfloat>(Screen::Width()),
		static_cast<GLfloat>(Screen::Height())};
	glUniform2fv(dimensionsI, 1, dimensions);

	// Draw the vignette.
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Clean up.
	glUseProgram(0);
}
