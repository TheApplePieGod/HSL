struct Material
{
    vec4 baseColor;
    vec4 emissiveFactor;
    vec4 texCoordTransform;
    vec4 hasPBRTextures;
    vec4 hasTextures;
    vec4 scalars;
};
struct Light
{
    vec4 position;
    vec4 direction;
    vec4 color;
    int lightType;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};
vec2 texCoord;
int entityId;
float depth;
vec3 worldPos;
vec3 normal;
vec3 tangent;
vec3 bitangent;
int instance;
uniform sampler2D albedoTex;
uniform sampler2D metallicRoughnessTex;
uniform sampler2D normalTex;
uniform sampler2D emissiveTex;
uniform sampler2D occlusionTex;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
};
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
};
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
};
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
};
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
};
vec4 GetAlbedo()
{
    vec4 color = materialBuffer.materials[instance].baseColor;
    if (materialBuffer.materials[instance].hasPBRTextures[0] == 1.f)
    {
        color *= texture(albedoTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy);
    };
    return color;
};
float GetMetalness()
{
    float metalness = materialBuffer.materials[instance].scalars[0];
    if (materialBuffer.materials[instance].hasPBRTextures[1] == 1.f)
    {
        metalness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).b;
    };
    return metalness;
};
float GetRoughness()
{
    float roughness = materialBuffer.materials[instance].scalars[1];
    if (materialBuffer.materials[instance].hasPBRTextures[1] == 1.f)
    {
        roughness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).g;
    };
    return roughness;
};
vec3 GetEmissive()
{
    vec3 emissive = materialBuffer.materials[instance].emissiveFactor.xyz;
    if (materialBuffer.materials[instance].hasTextures[1] == 1.f)
    {
        emissive *= texture(emissiveTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).rgb;
    };
    return emissive;
};
float GetOcclusion()
{
    float occlusion = 1.f;
    if (materialBuffer.materials[instance].hasTextures[2] == 1.f)
    {
        occlusion = texture(occlusionTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).r;
    };
    return occlusion;
};
void Clip(float alpha)
{
    if (alpha <= materialBuffer.materials[instance].scalars[2])
    {
        discard;
    };
};
vec4 GetFinalColor()
{
    vec4 baseColor = GetAlbedo();
    baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    Clip(baseColor.a);
    float roughness = GetRoughness();
    float metalness = GetMetalness();
    vec3 emissive = GetEmissive();
    float occlusion = GetOcclusion();
    vec3 V = normalize(frameBuffer.data.cameraPos.xyz - worldPos);
    vec3 nt = normalize(tangent);
    vec3 nb = normalize(bitangent);
    vec3 nn = normalize(normal);
    mat3 tbn = mat3(nb, nt, nn);
    vec3 N = nn;
    if (materialBuffer.materials[instance].hasTextures[0] == 1.f)
    {
        N = normalize(tbn * (texture(normalTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).xyz * 2.0 - 1.0));
    };
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, N);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metalness);
    vec3 nDfdx = dFdx(N.xyz);
    vec3 nDfdy = dFdy(N.xyz);
    float slopeSquare = max(dot(nDfdx, nDfdx), dot(nDfdy, nDfdy));
    float geometricRoughnessFactor = pow(clamp(slopeSquare, 0.0, 1.0), 0.333);
    float filteredRoughness = clamp(max(roughness, geometricRoughnessFactor) + 0.0005, 0.01f, 1.f);
    int lightCount = int(lightingBuffer.lights[0].position.x);
    vec3 finalContribution = vec3(0.f);
    for (int i = 1; i <= lightCount; i++)
    {
        float attenuation = 1.f;
        if (lightingBuffer.lights[i].lightType == POINT)
        {
            float dist = length(lightingBuffer.lights[i].position.xyz - worldPos);
            attenuation = 1.f / (lightingBuffer.lights[i].constantAttenuation + lightingBuffer.lights[i].linearAttenuation * dist + lightingBuffer.lights[i].quadraticAttenuation * dist * dist);
        };
        vec3 L = normalize(lightingBuffer.lights[i].position.xyz - worldPos);
        if (lightingBuffer.lights[i].lightType == DIRECTIONAL)
        {
            L = lightingBuffer.lights[i].direction.xyz;
        };
        vec3 H = normalize(V + L);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 lightColor = lightingBuffer.lights[i].color.rgb;
        float intensity = lightingBuffer.lights[i].color.a;
        vec3 radiance = lightColor * attenuation * intensity;
        float NDF = DistributionGGX(N, H, filteredRoughness);
        float G = GeometrySmith(N, V, L, filteredRoughness);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = min(vec3(1.f), numerator / denominator);
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness;
        float NdotL = max(dot(N, L), 0.0);
        finalContribution += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    };
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, filteredRoughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * baseColor.rgb;
    vec3 prefilteredColor = textureLod(prefilterMap, R, filteredRoughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), filteredRoughness)).rg;
    vec3 specular = min(vec3(1.f), prefilteredColor * (F * envBRDF.x + envBRDF.y));
    vec3 ambient = (kD * diffuse + specular) * occlusion;
    vec3 finalColor = ambient + finalContribution;
    return vec4(finalColor + emissive, baseColor.a);
};
