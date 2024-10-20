precision mediump float;

// The user must supply a texture and a color (white by default).
uniform sampler2D tex;
uniform vec4 color;

// This comes from the vertex shader.
in vec2 texCoord;

// Output color.
out vec4 finalColor;

// Multiply the texture by the user-specified color (including alpha).
void main() {
  finalColor = texture(tex, texCoord).a * color;
}
