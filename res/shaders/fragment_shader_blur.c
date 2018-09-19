in mediump vec4 colorOut;
in mediump vec2 texUV_out;

uniform sampler2D tex;
// uniform float resolution;
// uniform float radius;
// uniform vec2 dir;
out mediump vec4 color;

void main() {

	vec4 sum = vec4(0.0f);
	vec2 uv = texUV_out;
	uv.x = 0;


	//discard alpha for our simple demo, multiply by vertex color and return
	color = colorOut * sum;//vec4(sum.rgb, 1.0);
}