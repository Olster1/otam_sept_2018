uniform sampler2D normalMap;
in vec4 colorOut;
in vec2 texUV_out;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
	V3 ambient;
	V3 diffuse;
	V3 specular;

	V3 direction;
};

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, V4 colorShade)
{
    vec3 lightDir = normalize(-light.direction);
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results	
    V4 diffTex = texture(material.diffuse, texUV_out);
    V4 specTex = texture(material.specular, texUV_out);

    V4 ambColor = (diffTex*colorShade)*diffTex.w;
    V4 diffColor = ambColor
    V4 specColor = (specTex*colorShade)*specTex.w;

    vec4 ambient  = light.ambient  * ambColor
    vec4 diffuse  = light.diffuse  * diff * diffColor;
    vec4 specular = light.specular * spec * specColor;
    return (ambient + diffuse + specular);
}  

// vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
// {
//     vec3 lightDir = normalize(light.position - fragPos);
//     // diffuse shading
//     float diff = max(dot(normal, lightDir), 0.0);
//     // specular shading
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
//     // attenuation
//     float distance    = length(light.position - fragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + 
//   			     light.quadratic * (distance * distance));    
//     // combine results
//     vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
//     vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
//     vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
//     ambient  *= attenuation;
//     diffuse  *= attenuation;
//     specular *= attenuation;
//     return (ambient + diffuse + specular);
// } 

out vec4 color;
void main (void) {
    vec4 normal = texture(normalMap, texUV_out);
    
    vec4 b = colorOut*colorOut.w;

    vec3 lightingColor = CalcDirLight(light, normal.xyz, v3(0, 0, -1), b);
    color = vec4(lightingColor, 1);
}