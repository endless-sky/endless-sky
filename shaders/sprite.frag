precision mediump float;
precision mediump sampler2DArray;

uniform sampler2DArray tex;
uniform sampler2DArray swizzleMask;
uniform int useSwizzleMask;
uniform float frame;
uniform float frameCount;
uniform vec2 blur;
uniform int swizzler;
uniform float alpha;
const int range = 5;

in vec2 fragTexCoord;

out vec4 finalColor;

void main() {
  float first = floor(frame);
  float second = mod(ceil(frame), frameCount);
  float fade = frame - first;
  vec4 color;
  if(blur.x == 0.f && blur.y == 0.f)
  {
    if(fade != 0.f)
      color = mix(
        texture(tex, vec3(fragTexCoord, first)),
        texture(tex, vec3(fragTexCoord, second)), fade);
    else
      color = texture(tex, vec3(fragTexCoord, first));
  }
  else
  {
    color = vec4(0., 0., 0., 0.);
    const float divisor = float(range * (range + 2) + 1);
    for(int i = -range; i <= range; ++i)
    {
      float scale = float(range + 1 - abs(i)) / divisor;
      vec2 coord = fragTexCoord + (blur * float(i)) / float(range);
      if(fade != 0.f)
        color += scale * mix(
          texture(tex, vec3(coord, first)),
          texture(tex, vec3(coord, second)), fade);
      else
        color += scale * texture(tex, vec3(coord, first));
    }
  }
  vec4 swizzleColor;
  switch (swizzler) {
	// 0 red + yellow markings (republic)
    case 0:
      swizzleColor = color.rgba;
      break;
	// 1 red + magenta markings
    case 1:
      swizzleColor = color.rbga;
      break;
	// 2 green + yellow (free worlds)
    case 2:
      swizzleColor = color.grba;
      break;
	// 3 green + cyan
    case 3:
      swizzleColor = color.brga;
      break;
	// 4 blue + magenta (syndicate)
    case 4:
      swizzleColor = color.gbra;
      break;
	// 5 blue + cyan (merchant)
    case 5:
      swizzleColor = color.bgra;
      break;
	// 6 red and black (pirate)
    case 6:
      swizzleColor = color.gbba;
      break;
	// 7 pure red
    case 7:
      swizzleColor = color.rbba;
      break;
	// 8 faded red
    case 8:
      swizzleColor = color.rgga;
      break;
	// 9 pure black
    case 9:
      swizzleColor = color.bbba;
      break;
	// 10 faded black
    case 10:
      swizzleColor = color.ggga;
      break;
	// 11 pure white
    case 11:
      swizzleColor = color.rrra;
      break;
	// 12 darkened blue
    case 12:
      swizzleColor = color.bbga;
      break;
	// 13 pure blue
    case 13:
      swizzleColor = color.bbra;
      break;
	// 14 faded blue
    case 14:
      swizzleColor = color.ggra;
      break;
	// 15 darkened cyan
    case 15:
      swizzleColor = color.bgga;
      break;
	// 16 pure cyan
    case 16:
      swizzleColor = color.brra;
      break;
	// 17 faded cyan
    case 17:
      swizzleColor = color.grra;
      break;
	// 18 darkened green
    case 18:
      swizzleColor = color.bgba;
      break;
	// 19 pure green
    case 19:
      swizzleColor = color.brba;
      break;
	// 20 faded green
    case 20:
      swizzleColor = color.grga;
      break;
	// 21 darkened yellow
    case 21:
      swizzleColor = color.ggba;
      break;
	// 22 pure yellow
    case 22:
      swizzleColor = color.rrba;
      break;
	// 23 faded yellow
    case 23:
      swizzleColor = color.rrga;
      break;
	// 24 darkened magenta
    case 24:
      swizzleColor = color.gbga;
      break;
	// 25 pure magenta
    case 25:
      swizzleColor = color.rbra;
      break;
	// 26 faded magenta
    case 26:
      swizzleColor = color.rgra;
      break;
	// 27 red only (cloaked)
    case 27:
      swizzleColor = vec4(color.b, 0.f, 0.f, color.a);
      break;
	// 28 black only (outline)
    case 28:
      swizzleColor = vec4(0.f, 0.f, 0.f, color.a);
      break;
  }
  if(useSwizzleMask > 0)
  {
    float factor = texture(swizzleMask, vec3(fragTexCoord, first)).r;
    color = color * factor + swizzleColor * (1.0 - factor);
  }
  else
    color = swizzleColor;
  finalColor = color * alpha;
}
