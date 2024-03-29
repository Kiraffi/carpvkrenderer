#version 450
#include "common.h"

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

vec2 uvs[3] = vec2[](
    vec2(0.0, 0.0),
    vec2(2.0, 0.0),
    vec2(0.0, 2.0)
);


struct VData
{
    vec3 pos;
    uint color;
};

layout (set = 0, binding = 0, MATRIX_ORDER) restrict readonly buffer VertexDataBuffer
{
    VData vData[];
} vertData;

layout (location = 0) out vec4 colOut;
layout (location = 1) out vec2 uvOut;


void main()
{
    vec3 pos = vec3(positions[gl_VertexIndex], 0.0);

    colOut = vec4(1.0);
    uvOut = uvs[gl_VertexIndex];
    uvOut.y = 1.0 - uvOut.y;
    gl_Position = vec4(pos, 1.0);
}
