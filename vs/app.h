//  Copyright © 2021 Subph. All rights reserved.
//

#pragma once

#include "common.h"
#include "shader.h"
#include "mesh.h"
#include "buffer.h"
#include "image.h"
#include "camera.h"

#define WIDTH   800
#define HEIGHT  600

struct UniformBuffer {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class App {
public:
    
    void run();

private:

    GLFWwindow* m_window;

    std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
    std::vector<VkPresentModeKHR  > m_presentModes;

    uint32_t m_graphicQueueIndex = 0;
    uint32_t m_presentQueueIndex = 0;
    VkQueue m_graphicQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    void cleanup();
    
    void initWindow();
    void initVulkan();

    Mesh* m_pCube;
    Mesh* m_pPlane;
    Mesh* m_pQuad;
    void createGeometry();
    
    VkExtent2D m_extent;
    VkFormat m_surfaceFormat;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    void createSwapchain();

    VkRenderPass  m_renderPass = VK_NULL_HANDLE;
    void createRenderPass();

    uint32_t m_totalFrame = 0;
    Image*   m_depthImage;
    Buffer*  m_uniformBuffer;
    std::vector<VkCommandBuffer> m_cmdBuffers;
    std::vector<Image*>        m_fbImages;
    std::vector<VkFramebuffer> m_fb;
    std::vector<VkSemaphore>   m_imageSemaphores;
    std::vector<VkSemaphore>   m_renderSemaphores;
    std::vector<VkFence>       m_commandFences;
    void createFrameData();
    
    VkDescriptorPool             m_descPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout        m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet              m_descSet;
    VkDescriptorSetLayoutBinding m_descLayoutBinding;
    void createOffscreenDescriptorSet();
    void updateOffscreenDescriptorSet();


    UniformBuffer m_mvp{};
    uint32_t m_currentFrame = 0;
    Camera* m_camera;
    void process();
    
    VkDescriptorPool             m_postDescPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout        m_postDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet              m_postDescSet;
    VkDescriptorSetLayoutBinding m_postDescLayoutBinding;

    VkPipeline       m_postPipeline;
    VkPipelineLayout m_postPipelineLayout;

    void createPostDescriptor();
    void createPostPipeline();
    void updatePostDescriptorSet();

    // offscreen.cpp
    VkRenderPass m_offscreenRenderPass = VK_NULL_HANDLE;
    void createOffscreenRenderPass();

    Image*  m_offscreenImage;
    Image*  m_offscreenDepth;
    Buffer* m_offscreenUniformBuffer;
    VkFramebuffer m_offscreenFramebuffer;
    void createOffscreenFramedata();

    VkPipeline m_offscreenPipeline;
    VkPipelineLayout m_offscreenPipelineLayout;
    void createOffscreenPipeline();

    // vkray.cpp
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties;
    VkAccelerationStructureKHR m_blAccelStructure;
    VkAccelerationStructureKHR m_tlAccelStructure;

    VkDescriptorPool      m_rtDescPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_rtDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet       m_rtDescSet       = VK_NULL_HANDLE;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;

    VkPipeline       m_rtPipeline;
    VkPipelineLayout m_rtPipelineLayout;

    Buffer* m_rtSBTBuffer;

    struct RtPushConstant
    {
        glm::vec4 clearColor;
        glm::vec3 lightPosition;
        float     lightIntensity{ 100.0f };
        int       lightType{ 0 };
    } m_rtPushConstants;

    void initRayTracing();
    void createBottomLevelAS();
    void createTopLevelAS();
    void createRtDescriptorSet();
    void createRtPipeline();
    void createRtShaderBindingTable();
    
    // device.cpp
    std::vector<const char*> deviceExtensions;
    std::vector<const char*> validationLayers;

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;
    VkDevice     m_device   = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    void cleanupDevice();
    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    VkSurfaceFormatKHR getSwapchainSurfaceFormat();
    VkPresentModeKHR   getSwapchainPresentMode();

    // command.cpp
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    void createCommandPool();
    std::vector<VkCommandBuffer> createCommandBuffers( uint32_t count = 1 );
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands( VkCommandBuffer commandBuffer );


};
