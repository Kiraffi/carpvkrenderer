#pragma once


#include <carpvkcommon.h>

struct alignas(256) MyMemory
{
    Image m_firstPassRendertargetImage;
    Image m_lastPassRendertargetImage;

    // Indirect draw data.
    Buffer m_dynamicIndirectData;

    Buffer m_dynamicVertexData;
    Buffer m_dynamicIndexData;
    // Vertices in
    Buffer m_allVertexInData;
    Buffer m_allIndexInData;

    bool m_initialized;
};


bool initMemory();
void destroyMemory();
MyMemory& getMemory();

bool recreateRenderTargets(int newWidth, int newHeight);
