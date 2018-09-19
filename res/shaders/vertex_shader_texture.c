uniform samplerBuffer PVMArray;
uniform samplerBuffer ColorArray;
uniform samplerBuffer UVArray;
// uniform vec4 color;

in vec3 vertex;
in vec2 texUV;
//in int instanceIndex;

out vec4 colorOut; //out going
out vec2 texUV_out;
out float zAt;

void main() {
	
	int offset = 4 * int(gl_InstanceID);
	vec4 a = texelFetch(PVMArray, offset + 0);
	vec4 b = texelFetch(PVMArray, offset + 1);
	vec4 c = texelFetch(PVMArray, offset + 2);
	vec4 d = texelFetch(PVMArray, offset + 3);
	
	mat4 PVM = mat4(a, b, c, d);
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = texelFetch(ColorArray, gl_InstanceID);

    vec4 uvQuad = texelFetch(UVArray, gl_InstanceID);

    int xAt = int(texUV.x*2);
    int yAt = int(texUV.y*2) + 1;
    texUV_out = vec2(uvQuad[xAt], uvQuad[yAt]);

    zAt = gl_Position.z;
}