#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "mymemory.h"

#include <carpvk.h>
#include <common.h>


static MyMemory* sMyMemory = nullptr;

bool initMemory()
{
    sMyMemory = new MyMemory;
    sMyMemory->initialized = true;
    return true;
}

void destroyMemory()
{
    if(sMyMemory)
    {
        destroyImage(sMyMemory->m_firstPassRendertargetImage);
        destroyImage(sMyMemory->m_lastPassRendertargetImage);
        delete sMyMemory;
    }
    sMyMemory = nullptr;
}

MyMemory& getMemory()
{
    if(sMyMemory == nullptr)
    {
        ASSERT(sMyMemory);
    }
    return *sMyMemory;
}


bool recreateRenderTargets(int newWidth, int newHeight)
{
    if(newWidth == sMyMemory->m_firstPassRendertargetImage.width
        && newHeight == sMyMemory->m_firstPassRendertargetImage.height)
    {
        return true;
    }

    destroyImage(sMyMemory->m_firstPassRendertargetImage);
    destroyImage(sMyMemory->m_lastPassRendertargetImage);

    if(!createImage(newWidth, newHeight, getSwapChainFormats().defaultColorFormat,
        //VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        VK_IMAGE_USAGE_STORAGE_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT,
        "Render target0", sMyMemory->m_firstPassRendertargetImage))
    {
        printf("Failed to recreate render target image\n");
        return false;
    }

    if(!createImage(newWidth, newHeight, getSwapChainFormats().defaultColorFormat,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    "Render target1", sMyMemory->m_lastPassRendertargetImage))
    {
        printf("Failed to recreate render target image\n");
        return false;
    }

    return true;
}

