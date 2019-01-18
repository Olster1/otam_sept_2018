in vec3 vertex;
in vec4 texUV;	

in mat4 PVM;
in vec4 color;
in vec4 uvAtlas;	

//in int instanceIndex;

out vec4 colorOut; //out going
out vec2 texUV_out;
out float zAt;

void main() {
    
    gl_Position = PVM * vec4(vertex, 1);
    colorOut = color;
    
    int xAt = int(texUV.x*2);
    int yAt = int(texUV.y*2) + 1;
    texUV_out = vec2(uvAtlas[xAt], uvAtlas[yAt]);
    
    zAt = gl_Position.z;
}