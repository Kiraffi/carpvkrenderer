#include <vulkan/vulkan_core.h>
#include <stdio.h>

#include <carpvk.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <fstream>

#include "mymemory.h"

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;

enum ShaderTypes
{
    ShaderTypes_TwoDVert,
    ShaderTypes_TwoDFrag,
    ShaderTypes_FullScreenVert,
    ShaderTypes_FullScreenFrag,
    ShaderTypes_Compute,

    ShaderTypes_Count
};


static DescriptorSetLayout sGraphicsDescriptorLayouts[] =
{
    {
        .bindingIndex = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT
    },
    {
        .bindingIndex = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
    },
    {
        .bindingIndex = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stage = VK_SHADER_STAGE_VERTEX_BIT
            | VK_SHADER_STAGE_FRAGMENT_BIT
            | VK_SHADER_STAGE_COMPUTE_BIT,
        //not working properly .immutableSampler = state.m_sampler,
    },

};



struct State
{
    GLFWwindow* m_window = nullptr;
    VkDescriptorSetLayout m_descriptorSetLayout = {};

    VkPipelineLayout m_graphicsPipelineLayout = {};
    VkPipeline m_graphicsPipeline = {};

    VkPipeline m_secondGraphicsPipeline = {};

    VkPipeline m_computePipeline = {};

    VkShaderModule m_shaderModules[ShaderTypes_Count] = {};

    VkDescriptorSet m_descriptorSet = {};
    VkDescriptorSet m_descriptorSetForSecondPass = {};
    Buffer m_modelVerticesBuffer = {};
    UniformBuffer m_uniformBuffer = {};
    //Image m_image = {};
    Image m_sampledImage = {};

    VkSampler m_sampler = {};
    double m_ticksAtStart = {};
    //uint64_t m_ticksAtStart = {};
};


void sSleepMs(double time)
{
#ifdef WIN32
    Sleep((DWORD)(time));
#else
    usleep((useconds_t)(time*1000)); // unistd.h
#endif
}


static void sDeinitShaders(void* userData)
{
    State *state = (State *) userData;

    destroySampler(state->m_sampler);
    //destroyImage(state->m_image);
    destroyImage(state->m_sampledImage);

    destroyMemory();

    destroyShaderModule(state->m_shaderModules, ARRAYSIZES(state->m_shaderModules));
    destroyBuffer(state->m_modelVerticesBuffer);
    destroyPipelines(&state->m_computePipeline, 1);
    destroyPipelines(&state->m_graphicsPipeline, 1);
    destroyPipelines(&state->m_secondGraphicsPipeline, 1);
    destroyPipelineLayouts(&state->m_graphicsPipelineLayout, 1);

    destroyDescriptorSetLayouts(&state->m_descriptorSetLayout, 1);
}

static void sGetWindowSize(int32_t* widthOut, int32_t* heightOut, void* userData)
{
    const State* state = (State*) userData;

    glfwGetFramebufferSize(state->m_window, widthOut, heightOut);
}

static bool sUpdateRenderTargetDescriptorSets(State& state)
{
    const DescriptorInfo descritorSetInfos[] = {
        DescriptorInfo(state.m_modelVerticesBuffer, 0, state.m_modelVerticesBuffer.size),
        DescriptorInfo(state.m_uniformBuffer),
        DescriptorInfo(getMemory().m_firstPassRendertargetImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, state.m_sampler),
    };

    if(!updateBindDescriptorSet(state.m_descriptorSetForSecondPass,
        sGraphicsDescriptorLayouts, descritorSetInfos, ARRAYSIZES(descritorSetInfos)))
    {
        printf("Failed to update descriptorset\n");
        return false;
    }


    return true;
}

static bool sCreateRenderTargetImage(State& state)
{
    int width = 0;
    int height = 0;
    sGetWindowSize(&width, &height, &state);
    return recreateRenderTargets(width, height);
}

static bool sCreateImages(State& state)
{
    //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    static constexpr int width = 256;
    static constexpr int height = 256;
    if(!createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        "Sampler target", state.m_sampledImage))
    {
        printf("Failed to create sampler target image\n");
        return false;
    }

    std::vector<u32> picData;
    picData.resize(width * height);
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            u32 pixel = 255 << 24;
            pixel |= x << 0;
            pixel |= y << 8;
            picData[x + y * width] = pixel;
        }
    }

    uploadToImage(width, height, 4, state.m_sampledImage,
        picData.data(), picData.size() * 4u);


    return sCreateRenderTargetImage(state);
}


static void sResized(void* userData)
{
    State* state = (State*) userData;
    sCreateRenderTargetImage(*state) && sUpdateRenderTargetDescriptorSets(*state);
}

static VkSurfaceKHR sCreateSurface(VkInstance instance, void* userData)
{
    State* state = (State*)userData;
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(instance, state->m_window, nullptr, &surface);
    if(err || !surface)
    {
        printf("Failed to create surface\n");
        return {};
    }
    return surface;
}



static bool sDraw(State& state)
{
    VkCommandBuffer commandBuffer = getVkCommandBuffer();

    // Compute
    {
        struct UpdateStruct
        {
            uint32_t totalTimeMs;
            uint32_t padding[63];
        };


        UpdateStruct data = {
            .totalTimeMs = uint32_t((glfwGetTime() - state.m_ticksAtStart) * 1'000),
        };
        uploadToUniformBuffer(state.m_uniformBuffer, &data, sizeof(UpdateStruct));

        bufferBarrier(state.m_modelVerticesBuffer,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
        bufferBarrier(state.m_uniformBuffer,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT
                | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
                | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT);

        flushBarriers();

        beginComputePipeline(state.m_graphicsPipelineLayout, state.m_computePipeline, state.m_descriptorSet);
        vkCmdDispatch(commandBuffer, 1, 1, 1);
        endComputePipeline();
    }

    {
        imageBarrier(getMemory().m_firstPassRendertargetImage,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        
        bufferBarrier(state.m_modelVerticesBuffer,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

        flushBarriers();
    }

    RenderingAttachmentInfo firstPassColorAttachments[] =
    {
        {
            .clearValue = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } },
            .image = &getMemory().m_firstPassRendertargetImage,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        }
    };

    beginRenderPipeline(firstPassColorAttachments, ARRAYSIZES(firstPassColorAttachments), nullptr,
        state.m_graphicsPipelineLayout, state.m_graphicsPipeline, state.m_descriptorSet);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    endRenderPipeline();



    {
        //imageBarrier(state.m_sampledImage,
        //    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        imageBarrier(getMemory().m_firstPassRendertargetImage,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT
              | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
              //| VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
              ,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        imageBarrier(getMemory().m_lastPassRendertargetImage,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        flushBarriers();
    }

    RenderingAttachmentInfo lastPassColorAttachments[] =
    {
        {
            .clearValue = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } },
            .image = &getMemory().m_lastPassRendertargetImage,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        }
    };

    beginRenderPipeline(lastPassColorAttachments, ARRAYSIZES(lastPassColorAttachments), nullptr,
        state.m_graphicsPipelineLayout, state.m_secondGraphicsPipeline,
        state.m_descriptorSetForSecondPass);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    endRenderPipeline();


    return true;
}


float clamp(float v, float minValue, float maxValue)
{
    v = v < minValue ? minValue : v;
    v = v > maxValue ? maxValue : v;
    return v;
}

static uint32_t sGetColor(float r, float g, float b, float a)
{
    r = clamp(r, 0.0f, 1.0f);
    g = clamp(g, 0.0f, 1.0f);
    b = clamp(b, 0.0f, 1.0f);
    a = clamp(a, 0.0f, 1.0f);
    uint32_t color = uint32_t(r * 255.0f);
    color |= (uint32_t(g * 255.0f)) << 8u;
    color |= (uint32_t(b * 255.0f)) << 16u;
    color |= (uint32_t(a * 255.0f)) << 24u;
    return color;
}


static const char* const* sGetExtensions(uint32_t* outCount)
{
    return glfwGetRequiredInstanceExtensions(outCount);
}

static void sKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}


static void sErrorCB(int32_t error, const char* description)
{
    printf("Error: %s\n", description);
}




/////////
///
/// SRun
///
/////////
static int sRun(State &state)
{
    if (!glfwInit())
    {
        printf("Failed to initialize glfw\n");
        return -1;
    }
    glfwSetErrorCallback(sErrorCB);


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GLFW Window", nullptr, nullptr);

    // Check that the window was successfully created
    if (state.m_window == nullptr)
    {
        const char* error;
        glfwGetError(&error);
        printf("Could not create glfw window: %s\n", error);
        return false;
    }
    static const char *extensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;
    sGetWindowSize(&width, &height, &state);

    VulkanInstanceParams vkInstanceParams = {
        .getExtraExtensionsFn = sGetExtensions,
        .createSurfaceFn = sCreateSurface,
        .destroyBuffersFn = sDeinitShaders,
        .getWindowSizeFn = sGetWindowSize,
        .resizedFn = sResized,
        .userData = &state,
#if !NDEBUG
        .extensions = extensions,
        .extensionCount = ARRAYSIZES(extensions),
#endif
        .width = width,
        .height = height,
        .vsyncMode = VSyncType::IMMEDIATE_NO_VSYNC,
#if !NDEBUG
        .useValidation = true,
#endif
        .useIntegratedGpu = true, //true,
    };

    if(!initVulkan(vkInstanceParams))
    {
        printf("Failed to initialize vulkan\n");
        return 1;
    }
    if(!initMemory())
    {
        printf("Failed to initialize memory\n");
        return 1;
    }

    if(!createBuffer(1024u * 1024u * 16u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Model Vercies data buffer",
        state.m_modelVerticesBuffer))
    {
        printf("Failed to create model vertice data buffer\n");
        return 2;
    }

    state.m_sampler = createSampler({
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
    });

    {
        beginPreFrame();
        if(!sCreateImages(state))
        {
            printf("Failed to create images\n");
            return 8;
        }

        {
            struct VData
            {
                float posX;
                float posY;
                float posZ;
                uint32_t color;
            };

            VData vData[] =
            {
                {0.0, 0.5f, 0.5f, sGetColor(0.0f, 1.0f, 0.0f, 1.0f)},
                {-0.5, -0.5f, 0.5f, sGetColor(1.0f, 0.0f, 0.0f, 1.0f)},
                {0.5, -0.5f, 0.5f, sGetColor(0.0f, 0.0f, 1.0f, 1.0f)},

                {0.0, 0.5f, 0.5f, sGetColor(0.0f, 1.0f, 0.0f, 1.0f)},
                {0.5, -0.5f, 0.5f, sGetColor(0.0f, 0.0f, 1.0f, 1.0f)},
                {0.5, 0.5f, 0.5f, sGetColor(1.0f, 0.0f, 0.0f, 1.0f)},

            };
            uploadToGpuBuffer(state.m_modelVerticesBuffer, vData, 0, sizeof(vData));
            imageBarrier(state.m_sampledImage,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            flushBarriers();
        }
        endPreFrame();
    }

    state.m_descriptorSetLayout =
        createSetLayout(sGraphicsDescriptorLayouts, ARRAYSIZES(sGraphicsDescriptorLayouts));

    if(!createDescriptorSet(state.m_descriptorSetLayout, &state.m_descriptorSet))
    {
        printf("Failed to create descriptorset\n");
        return 2;
    }

    if(!createDescriptorSet(state.m_descriptorSetLayout, &state.m_descriptorSetForSecondPass))
    {
        printf("Failed to create descriptorset\n");
        return 2;
    }

    state.m_uniformBuffer = createUniformBuffer(256);


    state.m_graphicsPipelineLayout = createPipelineLayout(state.m_descriptorSetLayout);

    static const DescriptorInfo descritorSetInfos[] = {
        DescriptorInfo(state.m_modelVerticesBuffer, 0, state.m_modelVerticesBuffer.size),
        DescriptorInfo(state.m_uniformBuffer),
        DescriptorInfo(state.m_sampledImage.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, state.m_sampler),
    };

    if(!updateBindDescriptorSet(state.m_descriptorSet,
        sGraphicsDescriptorLayouts, descritorSetInfos, ARRAYSIZES(descritorSetInfos)))
    {
        printf("Failed to update descriptorset\n");
        return 1;
    }

    if(!sUpdateRenderTargetDescriptorSets(state))
    {
        printf("Failed to update descriptorset2\n");
        return 1;
    }

    if(!createShader("assets/shader/vert.spv", state.m_shaderModules[ShaderTypes_TwoDVert])
        || !createShader("assets/shader/frag.spv", state.m_shaderModules[ShaderTypes_TwoDFrag])

        || !createShader("assets/shader/full_screen_vert.spv", state.m_shaderModules[ShaderTypes_FullScreenVert])
        || !createShader("assets/shader/full_screen_frag.spv", state.m_shaderModules[ShaderTypes_FullScreenFrag])

        || !createShader("assets/shader/compshader.spv", state.m_shaderModules[ShaderTypes_Compute])
    )
    {
        printf("Failed to create shaders!\n");
        return 9;
    }

    const VkFormat colorFormats[] = { getSwapChainFormats().defaultColorFormat };

    const VkPipelineShaderStageCreateInfo stageInfos[] = {
        createDefaultVertexInfo(state.m_shaderModules[ShaderTypes_TwoDVert]),
        createDefaultFragmentInfo(state.m_shaderModules[ShaderTypes_TwoDFrag]),
    };

    GPBuilder gpBuilder = {
        .stageInfos = stageInfos,
        .colorFormats = colorFormats,
        .blendChannels = &cDefaultBlendState,
        .pipelineLayout = state.m_graphicsPipelineLayout,
        .stageInfoCount = ARRAYSIZES(stageInfos),
        .colorFormatCount = ARRAYSIZES(colorFormats),
        .blendChannelCount = 1,
    };

    state.m_graphicsPipeline = createGraphicsPipeline(gpBuilder, "Triangle graphics pipeline");

    if(!state.m_graphicsPipeline)
    {
        printf("Failed to create graphics pipeline\n");
        return 8;
    }

    const VkPipelineShaderStageCreateInfo secondStageInfos[] = {
        createDefaultVertexInfo(state.m_shaderModules[ShaderTypes_FullScreenVert]),
        createDefaultFragmentInfo(state.m_shaderModules[ShaderTypes_FullScreenFrag]),
    };

    gpBuilder.stageInfos = secondStageInfos;
    state.m_secondGraphicsPipeline = createGraphicsPipeline(gpBuilder, "Second triangle graphics pipeline");

    if(!state.m_secondGraphicsPipeline)
    {
        printf("Failed to create graphics pipeline\n");
        return 8;
    }


    CPBuilder cpBuilder = {
        .stageInfo = createDefaultComputeInfo(state.m_shaderModules[ShaderTypes_Compute]),
        .pipelineLayout = state.m_graphicsPipelineLayout,
    };

    state.m_computePipeline = createComputePipeline(cpBuilder, "Compute pipeline");
    if(!state.m_computePipeline)
    {
        printf("Failed to create compute pipeline\n");
        return 2;
    }

    glfwSetKeyCallback(state.m_window, sKeyCallback);



    char tmpBuf[256] = {};
    int updateTick = 0;
    double lastTicks = glfwGetTime();
    state.m_ticksAtStart = lastTicks;
    bool quit = false;
    //While application is running
    while( !quit && !glfwWindowShouldClose(state.m_window))
    {
        glfwPollEvents();
        beginFrame();

        sDraw(state);
        presentImage(getMemory().m_lastPassRendertargetImage);

        static int MaxTicks = 100;
        if(++updateTick >= MaxTicks)
        {
            double newTicks = glfwGetTime();
            double diff = newTicks - lastTicks;
            diff = diff > 0 ? diff / MaxTicks : 1;
            lastTicks = newTicks;

            snprintf(tmpBuf, 255, "FPS: %f - %fms", 1.0 / diff, diff / 1.0);
            glfwSetWindowTitle(state.m_window, tmpBuf);
            updateTick = 0;
        }
        sSleepMs(1);
    }

    printf("success\n");
    return 0;
}



int main()
{
    State state = {};
    int returnValue = sRun(state);

    deinitVulkan();

    glfwDestroyWindow(state.m_window);
    glfwTerminate();

    return returnValue;
}
