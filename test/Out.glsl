// BEGIN PREDEFINITIONS
// #########################
float saturate(float v) { return clamp(v, 0.f, 1.f); }
vec2 saturate(vec2 v) { return clamp(v, vec2(0.f), vec2(1.f)); }
vec3 saturate(vec3 v) { return clamp(v, vec3(0.f), vec3(1.f)); }
vec4 saturate(vec4 v) { return clamp(v, vec4(0.f), vec4(1.f)); }
// #########################

#version 460

// BEGIN INCLUDE (FrameBuffer.hsl)
// #########################
struct FrameData
{
    mat4 proj;
    mat4 view;
    vec4 cameraPos;
    vec2 screenSize;
    bool reverseDepth;
    float bloomThreshold;
    bool cullEnable;
    bool padding1;
    vec2 padding2;
};
layout(binding=0) uniform BUFFER0{ FrameData data[]; } frameBuffer;
// #########################

struct Instance
{
    vec4 objectId;
};
layout(binding=12) buffer BUFFER1{ Instance data[]; } instanceBuffer;
layout(location=0) out vec2 texCoord;
layout(location=1) out int entityId;
layout(location=2) out float depth;
layout(location=3) out vec3 worldPos;
layout(location=4) out vec3 normal;
layout(location=5) out vec3 tangent;
layout(location=6) out vec3 bitangent;
layout(location=7) out int instance;
void main()
{
    int objectId = (gl_BaseInstance + gl_InstanceID);
    if (frameBuffer.data[0].cullEnable)
    {
        objectId = int(instanceBuffer.data[(gl_BaseInstance + gl_InstanceID)].objectId.x);
    };
    worldPos = (objectBuffer[objectId].model * vec4(inPosition, 1.0)).xyz;
    vec4 viewPos = (frameBuffer.data[0].view * vec4(worldPos, 1.0));
    depth = viewPos.z;
    gl_Position = frameBuffer.data[0].proj * viewPos;
    texCoord = inTexCoord;
    entityId = int(objectBuffer[objectId].data[0]);
    instance = objectId;
    normal = mat3(objectBuffer[objectId].model) * inNormal;
    tangent = mat3(objectBuffer[objectId].model) * inTangent.xyz;
    bitangent = cross(inTangent.xyz, inNormal);
    bitangent *= inTangent.w;
};
