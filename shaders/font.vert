// scale maps pixel coordinates to GL coordinates (-1 to 1).
uniform vec2 scale;
// The (x, y) coordinates of the top left corner of the glyph.
uniform vec2 position;
// The glyph to draw. (ASCII value - 32).
uniform int glyph;
// Aspect ratio of rendered glyph (unity by default).
uniform float aspect;

// Inputs from the VBO.
in vec2 vert;
in vec2 corner;

// Output to the fragment shader.
out vec2 texCoord;

// Pick the proper glyph out of the texture.
void main() {
	texCoord = vec2((float(glyph) + corner.x) / 98.f, corner.y);
	gl_Position = vec4((aspect * vert.x + position.x) * scale.x, (vert.y + position.y) * scale.y, 0.f, 1.f);
}