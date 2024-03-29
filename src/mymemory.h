#pragma once


#include <carpvkcommon.h>

struct alignas(256) MyMemory
{
    Image m_firstPassRendertargetImage;
    Image m_lastPassRendertargetImage;

    bool initialized;
};


bool initMemory();
void destroyMemory();
MyMemory& getMemory();

bool recreateRenderTargets(int newWidth, int newHeight);
