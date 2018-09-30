// uniform sampler2D tex;
// in vec4 colorOut;
// in vec2 texUV_out;
// in float zAt;

out vec4 color;
void main (void) {
	// vec4 texColor = texture(tex, texUV_out);
	// float alpha = 1.0;//smoothstep(1, -0.4, zAt); 
	// float a = zAt;
	// alpha *= texColor.w;
	// texColor *= alpha;
    // color = colorOut*texColor;
    color = vec4(1, 0, 0, 1);

}