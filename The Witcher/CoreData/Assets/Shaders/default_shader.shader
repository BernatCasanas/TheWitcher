#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 uvs;
layout(location = 2) in vec3 normals; 
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;
layout (location = 5) in vec3 tangents;
layout (location = 6) in vec3 biTangents;

const int MAX_BONES = 100;
const int MAX_SPACEMATRIX = 10;

uniform mat4 gBones[MAX_BONES];
uniform mat4 view; 
uniform mat4 model;
uniform mat4 projection;

uniform int num_space_matrix;
uniform mat4 lightSpaceMatrix[MAX_SPACEMATRIX];
uniform mat4 lightSpaceMatrixBaked[MAX_SPACEMATRIX];
uniform int animate;

uniform float density = 0;
uniform float gradient = 0;

out vec3 frag_pos;
out vec2 texCoords;
out vec3 norms;
out mat3 TBN; 
out float visibility; 
out vec4 FragPosLightSpace[MAX_SPACEMATRIX];
out vec4 FragPosLightSpaceBaked[5];
//out float visibility;

uniform vec4 clip_plane;

void main()
{
    // --------------- OUTS ---------------
    vec4 pos = vec4(position, 1.0);
    texCoords = vec2(uvs.x, uvs.y);

    // --------- Fog ----------
    vec4 worldPos = model * pos;
    vec4 positionRelativeToCam = view * worldPos;
    float distance = length(positionRelativeToCam.xyz);

    visibility = exp(-pow((distance * density), gradient));
    visibility = clamp(visibility, 0.0, 1.0);
    // ------------------------
    gl_ClipDistance[0] = dot(worldPos, clip_plane);
    //gl_ClipDistance[0] = -1;
    // --------------------------------------- 

    // --------------- Animation -------------
    vec3 blendpos = vec3(0,0,0);
    if(animate == 1)
    {
         for(int i=0; i<4; i++)
        {
            blendpos += vec3(gBones[BoneIDs[i]] * pos) * Weights[i];
        }
            pos = vec4(blendpos,1);
    }
    // --------------------------------------- 
    frag_pos = vec3(model * pos);
    
    for(int i = 0; i < num_space_matrix; i++)
    {
        FragPosLightSpace[i] = lightSpaceMatrix[i] * vec4(frag_pos, 1.0);
        for(int j = 0; j < 3; j++)
        {
            int iterator = i*3 + j;
            FragPosLightSpaceBaked[iterator] = lightSpaceMatrixBaked[iterator] * vec4(frag_pos, 1.0);
        }
    }

    // --------------- Normals ---------------
    norms = mat3(transpose(inverse(model))) * normals;

    vec3 T = normalize(vec3(model * vec4(tangents,   0.0)));
    vec3 B = normalize(vec3(model * vec4(biTangents, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normals,    0.0)));

    TBN = mat3(T,B,N);
    // ---------------------------------------

    gl_Position = projection * view * vec4(frag_pos, 1.0f);
};


#shader fragment
#version 330 core

// Properties array index
#define indexPosition 0
#define indexAmbient 1
#define indexDiffuse 2
#define indexSpecular 3
#define indexDirection 4

// Structs
struct DirectionalLight
{
    float intensity;
    vec3 dirLightProperties[5];
    sampler2D depthMap;
    vec3 lightPos;

    float shadowIntensity;
    sampler2D bakeShadows[3];
    vec3 lightPosBaked[3];

    bool castShadow;
};

struct PointLight
{
    float intensity;
    vec3 pointLightProperties[4];
    float constant;
    float linear;
    float quadratic;

    bool affectShadows;
};

struct SpotLight
{
    float intensity;
    vec3 spotLightProperties[5];
    float constant;
    float linear;
    float quadratic;
    float cut_off;
    float outer_cut_off;
};

struct Material {
    vec4 diffuse_color;

    sampler2D diffuseTexture;
    bool hasDiffuseTexture;

    sampler2D specularMap;
    bool hasSpecularMap;

    sampler2D normalMap;
    bool hasNormalMap;

    float smoothness;
    float metalness;
    bool emissive;
};

// Function declarations
vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_dir, Material objectMaterial, vec2 texCoords, vec4 lightSpaceMat, vec4 lightSpaceMatBaked0, vec4 lightSpaceMatBaked1, vec4 lightSpaceMatBaked2, inout float shadowIntensity);
vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, Material objectMat, vec2 texCoords);
vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, Material objectMat, vec2 texCoords);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D depthMap);

// Uniforms
uniform Material objectMaterial;
// max_dir_lights, max_point_lights, max_spot_lights
uniform ivec3 max_lights;

uniform vec3 view_pos;

uniform float bloom_threshold = 1;
uniform bool activeFog;
uniform vec3 backgroundColor;

#define MAX_SPACEMATRIX 10
#define MAX_LIGHTS_PER_TYPE 10

uniform DirectionalLight dir_light[MAX_LIGHTS_PER_TYPE];
uniform PointLight point_light[50];
uniform SpotLight spot_light[MAX_LIGHTS_PER_TYPE];

// Ins
in vec2 texCoords;
in vec3 frag_pos;
in vec3 norms;
in mat3 TBN;
in float visibility;
in vec4 FragPosLightSpace[MAX_SPACEMATRIX];
in vec4 FragPosLightSpaceBaked[5];
// Outs
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main()
{
    // ----------------------- Object Color ---------------------
    vec4 objectColor = objectMaterial.diffuse_color;
    if(objectMaterial.hasDiffuseTexture == true)
    {
        objectColor = objectColor * vec4(texture(objectMaterial.diffuseTexture, texCoords));
    }

    if(objectColor.w < 0.001)
    {
        discard;
    }
    // ------------------------ Normals --------------------------------

    vec3 normal = vec3(0);
    if(objectMaterial.hasNormalMap == true)
    {
        normal = texture(objectMaterial.normalMap, texCoords).rgb;
        normal = normal * 2.0 - 1.0;
        // normal = normal * vec3(-1, -1, 0);
        normal = normalize(TBN * normal);
    }
    else 
    {
        normal = normalize(norms);
    }

    // ------------------------- Light --------------------------

    vec3 result = vec3(0);
    vec3 view_dir = normalize(view_pos - frag_pos);

    float intensityInShadows = 1.0;
    // Light calculations
    for(int i = 0; i < max_lights.x; i++)
    {
        int it0 = i*3;
        int it1 = i*3 + 1;
        int it2 = i*3 + 2;
        result += CalculateDirectionalLight(dir_light[i], normal, view_dir, objectMaterial, texCoords, FragPosLightSpace[i],FragPosLightSpaceBaked[it0], FragPosLightSpaceBaked[it1], FragPosLightSpaceBaked[it2], intensityInShadows);
    }
    for(int i = 0; i < max_lights.y; i++)
    {
        if(!point_light[i].affectShadows)
        {
            result += CalculatePointLight(point_light[i], normal, frag_pos, view_dir, objectMaterial, texCoords) * vec3(intensityInShadows);   
        }
        else
        {
            result += CalculatePointLight(point_light[i], normal, frag_pos, view_dir, objectMaterial, texCoords);
        }
    }
    for(int i = 0; i < max_lights.z; i++)
        result += CalculateSpotLight(spot_light[i], normal, frag_pos, view_dir, objectMaterial, texCoords) * vec3(intensityInShadows);   
    // ----------------------------------------------------------

    FragColor = vec4(result, 1.0) * objectColor;

    if(activeFog == true)
    {
        FragColor = mix(vec4(backgroundColor, 1.0), FragColor, visibility);
    }

    // Write in the 2nd color buffer if the output color is higher than a threshold
    float brightness = dot(vec3(FragColor.rgb * bloom_threshold), vec3(vec3(0.2126, 0.7152, 0.0722)));
    if(brightness > 1.0 && objectMaterial.emissive)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

}

// Function definitions
vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_dir, Material objectMaterial, vec2 texCoords, vec4 lightSpaceMat, vec4 lightSpaceMatBaked0, vec4 lightSpaceMatBaked1, vec4 lightSpaceMatBaked2, inout float shadowIntensity)
{
    // Intensity
    float intensity = light.intensity;

    // Ambient
    vec3 ambient = light.dirLightProperties[indexAmbient];
    
    // Diffuse
    vec3 fake_lightDir = normalize(light.lightPos - frag_pos);
    vec3 lightDir = normalize(-light.dirLightProperties[indexDirection]);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = light.dirLightProperties[indexDiffuse] * diff;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + view_dir); 
    float spec = pow(max(dot(normal, halfwayDir), 0.0), objectMaterial.smoothness) * objectMaterial.metalness;

    vec3 specular = vec3(0);
    if(objectMaterial.hasSpecularMap == true)
        specular = light.dirLightProperties[indexSpecular] * spec * vec3(texture(objectMaterial.specularMap, texCoords));
    else   
        specular = light.dirLightProperties[indexSpecular] * spec;


    // Final Calculation 
    vec3 result = vec3(0);

    //calculate shadows
    if(light.castShadow == true && specular != vec3(0.0) && diffuse != vec3(0.0))
    {
        float shadow = ShadowCalculation(lightSpaceMat, normal, fake_lightDir, light.depthMap);
        
        vec3 fake_lightDirBaked0 = normalize(light.lightPosBaked[0] - frag_pos);
        vec3 fake_lightDirBaked1 = normalize(light.lightPosBaked[1] - frag_pos);
        vec3 fake_lightDirBaked2 = normalize(light.lightPosBaked[2] - frag_pos);


        shadow += ShadowCalculation(lightSpaceMatBaked0, normal, fake_lightDirBaked0, light.bakeShadows[0]);
        shadow += ShadowCalculation(lightSpaceMatBaked1, normal, fake_lightDirBaked1, light.bakeShadows[1]);
        shadow += ShadowCalculation(lightSpaceMatBaked2, normal, fake_lightDirBaked2, light.bakeShadows[2]);
        
        if(shadow > 1.0)
            shadow = 1.0;
        if(shadow == 1)
        {
            shadowIntensity = 1.0 - light.shadowIntensity;
            result = ambient  * vec3(shadowIntensity) * vec3(intensity);
        }
        else
        {
            result = (ambient + diffuse + specular) * vec3(intensity, intensity, intensity);
        }

    }
    else 
    {
        result = (ambient + diffuse + specular) * vec3(intensity, intensity, intensity);
    }

    return result;
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, Material objectMaterial, vec2 texCoords)
{
    // Intensity
    float intensity = light.intensity;

    // Ambient
    vec3 ambient = light.pointLightProperties[indexAmbient];

    // diffuse shading
    vec3 lightDir = normalize(light.pointLightProperties[indexPosition] - frag_pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.pointLightProperties[indexDiffuse] * diff;

     // Specular
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(view_dir, reflectDir), 0.0), objectMaterial.smoothness) * objectMaterial.metalness;
        
    vec3 specular = vec3(0);
    if(objectMaterial.hasSpecularMap == true)
        specular = light.pointLightProperties[indexSpecular] * spec * vec3(texture(objectMaterial.specularMap, texCoords));
    else 
        specular = light.pointLightProperties[indexSpecular] * spec;

    // attenuation
    float distance = length(light.pointLightProperties[indexPosition] - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    return (ambient + diffuse + specular) * attenuation * vec3(intensity, intensity, intensity);
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, Material objectMaterial, vec2 texCoords)
{
    // Intensity
    float intensity = light.intensity;

    // ambient
    vec3 ambient = light.spotLightProperties[indexAmbient];
    
    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.spotLightProperties[indexPosition] - frag_pos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.spotLightProperties[indexDiffuse] * diff;  
    
    // specular
    vec3 viewDir = normalize(view_pos - frag_pos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), objectMaterial.smoothness) * objectMaterial.metalness;

    vec3 specular = vec3(0);
    if(objectMaterial.hasSpecularMap == true)
        specular = light.spotLightProperties[indexSpecular] * spec * vec3(texture(objectMaterial.specularMap, texCoords));  
    else
        specular = light.spotLightProperties[indexSpecular] * spec;  

    // soft edges
    float theta = dot(lightDir, normalize(-light.spotLightProperties[indexDirection])); 
    float epsilon = (light.cut_off - light.outer_cut_off);
    float edge_intensity = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);
    diffuse  *= edge_intensity;
    specular *= edge_intensity;
    
    // attenuation
    float distance = length(light.spotLightProperties[indexPosition] - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    ambient *= attenuation; 
    diffuse *= attenuation;
    specular *= attenuation;   
        
    return (ambient + diffuse + specular) * intensity;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D depthMap)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
        
    float shadow = 0.0;
    vec2 texelSize = 1.0/ textureSize(depthMap,0);

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(depthMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - 0.002 > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
       shadow = 0.0;
       
    return shadow;
}