uniform sampler2D tex;
in mediump vec4 colorOut;
in mediump vec2 texUV_out;
in mediump float zAt;

out vec4 color;
void main (void) {
	vec4 texColor = texture(tex, texUV_out);
	float alpha = 1;//smoothstep(1, -0.4, zAt); 
	float a = zAt;
	alpha *= texColor.w;
	texColor *= alpha;
    color = colorOut*texColor;
}