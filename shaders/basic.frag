#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

// fog
uniform float fogDensity;
//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform sampler2D shadowMap;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//components
vec3 ambient;
float ambientStrength = 0.2;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5;
vec4 fPosEye;

float constant = 1.0;
float linear = 0.0045;
float quadratic = 0.0075;
float shininess = 12.0;

// spotlight (flash)
uniform vec3 cameraFront;
uniform vec3 cameraPos;
uniform bool flash;
uniform bool grey;

vec3 ambientf;
vec3 diffusef;
vec3 specularf;

vec3 _ambient = vec3( 0.1f, 0.1f, 0.1f);
vec3 _diffuse = vec3(0.8f, 0.8f, 0.8f);
vec3 _specular = vec3(1.0f, 1.0f, 1.0f);

float attenuation;
float cutOff = 0.9763;
float outerCutOff = 0.9537;

void computeDirLight() {
    //compute eye space coordinates
	fPosEye = view * model * vec4(fPosition, 1.0);
	vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
	vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0)));

    //compute view direction in eye coordinates, the viewer is situated at the origin
	vec3 viewDir = normalize(-fPosEye.xyz);

    //compute distance to light
	float dist = length(lightDir - fPosEye.xyz);
	//compute attenuation
	float att = 1.0 / (constant + linear * dist + quadratic * (dist * dist));

    //compute ambient light
	ambient = att * ambientStrength * lightColor;

    //compute diffuse light
	diffuse = att * max(dot(normalEye, lightDirN), 0.0) * lightColor;

    //compute specular light
	vec3 reflectDir = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	specular = att * specularStrength * specCoeff * lightColor;
}

void computeFlash(){

	ambientf = _ambient * texture(diffuseTexture, fTexCoords).rgb;

	vec3 norm = normalize(fNormal);
	vec3 lightDirN = normalize(cameraPos - fPosition);
	float diff = max(dot(norm, lightDirN), 0.0f);
	diffusef = _diffuse * diff * texture(diffuseTexture, fTexCoords).rgb;

	vec3 viewDirN = normalize(cameraPos - fPosition);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDirN, reflectDir), 0.0), shininess);
    specularf = _specular * spec * texture(diffuseTexture, fTexCoords).rgb;

	float theta = dot(lightDirN, normalize(-cameraFront));
	float epsilon = cutOff - outerCutOff;
	float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);  
	diffuse *= intensity;
	specular *=  intensity;

	float distance = length(cameraPos - fPosition);
    attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
}



float computeShadow() {
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	if(normalizedCoords.z > 1.0)
		return 0.0;

	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;

	// Check whether current frag pos is in shadow
	float bias = 0.005;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

float computeFog() {
	float fragmentDistance = length(fPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

	return clamp(fogFactor, 0.0, 1.0);
}

void main() {

	computeDirLight();

	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	// compute spot light
	
	if(flash){
		computeFlash();
		ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;
	}
		

	// modulate with shadow
	float shadow = computeShadow();

	// modualte with fog
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5, 0.5, 0.5, 1.0);
	vec3 color = min((ambient + (1.0 - shadow) * diffuse) + (1.0 - shadow) * specular, 1.0);

	if(grey){
		color = vec3(vec3(color).y);
	}
	fColor = mix(fogColor, vec4(color, 1.0), fogFactor);

}
