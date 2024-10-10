precision mediump float;
precision mediump sampler2D;

uniform sampler2D tex;
uniform vec4 fade;

in vec2 tpos;
in vec2 vpos;
out vec4 finalColor;

void main() {
	float epsilon = .001;  float weightTop = clamp((vpos.y + epsilon) / (fade[0] + epsilon), 0.0, 1.0);
	float weightBottom = clamp(((1.0 - vpos.y) + epsilon) / (fade[1] + epsilon), 0.0, 1.0);
	float weightLeft = clamp((vpos.x + epsilon) / (fade[2] + epsilon), 0.0, 1.0);
	float weightRight = clamp(((1.0 - vpos.x) + epsilon) / (fade[3] + epsilon), 0.0, 1.0);
	float weight = min(min(min(weightTop, weightBottom), weightLeft), weightRight);
	if(tpos.x > 0.0 && tpos.y > 0.0 &&
		tpos.x < 1.0 && tpos.y < 1.0 )
	finalColor = texture(tex, tpos) * weight;
	else
		discard;
}