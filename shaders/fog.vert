uniform vec2 corner;
uniform vec2 dimensions;

in vec2 vert;
out vec2 fragTexCoord;

void main() {
	gl_Position = vec4(corner + vert * dimensions, 0, 1);
	fragTexCoord = vert;
}
