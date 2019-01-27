uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;
in float zAt;

out vec4 color;
void main (void) {
	vec4 texColor = texture(tex, texUV_out);
	float alpha = texColor.w;
	if(alpha == 0) discard; 
	float a = zAt;
	vec4 b = colorOut*colorOut.w;
	vec4 c = b*texColor;
	c *= alpha;
    color = c;
}