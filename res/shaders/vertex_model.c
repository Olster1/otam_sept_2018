uniform mat4 perspective;
uniform mat4 view;
uniform mat4 model;

// uniform vec4 color;

in vec3 vertex;
// in vec2 texUV;
// in vec3 normal;
//in int instanceIndex;

// out vec4 colorOut; //out going
// out vec2 texUV_out;

void main() {
    gl_Position = perspective * view * model * vec4(vertex, 1);
    // colorOut = texelFetch(ColorArray, gl_InstanceID);
    // texUV_out = texUV;
}