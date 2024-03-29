#version 450
#include "common.h"

vec2 positions[3] = vec2[](
    vec2(0.5, -0.5),
    vec2(0.0, 0.5),
    vec2(-0.5, -0.5)
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
    vec3 pos = vertData.vData[gl_VertexIndex].pos;
    uint col = vertData.vData[gl_VertexIndex].color;
    gl_Position = vec4(pos, 1.0);
    uvOut = pos.xy * 0.5 + 0.5;
    colOut = vec4(
        (col & 255u),
        ((col >> 8u) & 255u),
        ((col >> 16u) & 255u),
        ((col >> 25u) & 255u));

    colOut /= 255.0;
}
