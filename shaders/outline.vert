uniform vec2 scale;
uniform vec2 position;
uniform mat2 transform;

in vec2 vert;
in vec2 vertTexCoord;
out vec2 fragTexCoord;

void main() {
	fragTexCoord = vertTexCoord;
	gl_Position = vec4((transform * vert + position) * scale, 0, 1);
}