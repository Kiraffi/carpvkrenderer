#version 450

layout(location = 0) out vec4 outColor;
layout (location = 0) in vec4 colIn;
layout (location = 1) in vec2 uvIn;

layout (binding = 2) uniform sampler2D textureToSample;




void main()
{
    outColor = texture(textureToSample, uvIn);
}
