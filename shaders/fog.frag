precision mediump sampler2D;
precision mediump float;
uniform sampler2D tex;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
    finalColor = vec4(0, 0, 0, texture(tex, fragTexCoord).r);
}