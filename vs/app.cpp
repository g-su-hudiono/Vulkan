//  Copyright © 2021 Subph. All rights reserved.
//

#include <array>

#include "app.h"
#include "helper.h"

#define WIDTH   800
#define HEIGHT  600

void App::run() {
    initWindow();
    initVulkan();
    process();
    cleanup();
}

void App::cleanup() {

    m_vertexShader->cleanup();
    m_pixelShader->cleanup();

    m_pCube->cleanup();
    m_pFloor->cleanup();

    for ( size_t i = 0; i < m_imageSemaphores.size(); i++ ) {
        vkDestroySemaphore( m_device, m_imageSemaphores[i], nullptr );
    }

    for ( size_t i = 0; i < m_totalFrame; i++ ) {
        vkWaitForFences( m_device, 1, &m_commandFences[i], VK_TRUE, UINT64_MAX );

        vkDestroyFence( m_device, m_commandFences[i], nullptr );
        vkDestroySemaphore( m_device, m_renderSemaphores[i], nullptr );
        vkDestroyFramebuffer( m_device, m_fb[i], nullptr );

        m_uniformBuffer[i]->cleanup();
        m_fbImages[i]->cleanupImageView();
    }
    m_depthImage->cleanup();

    vkDestroyRenderPass( m_device, m_renderPass, nullptr );
    vkDestroySwapchainKHR( m_device, m_swapchain, nullptr );

    vkDestroyPipeline( m_device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );

    vkDestroyDescriptorSetLayout( m_device, m_descSetLayout, nullptr );
    vkDestroyDescriptorPool( m_device, m_descPool, nullptr );

    vkDestroyCommandPool( m_device, m_commandPool, nullptr );
    
    cleanupDevice();

    glfwSetWindowShouldClose( m_window, true );
    glfwDestroyWindow( m_window );
    glfwTerminate();
}

void App::initWindow() {
    glfwInit();
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    m_window = glfwCreateWindow( WIDTH, HEIGHT, "RT", nullptr, nullptr);
    glfwSetWindowUserPointer( m_window, this );

}

void App::initVulkan() {
    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    
    createShader();
    createGeometry();

    createSwapchain();
    createRenderPass();

    createFrameData();

    createDescriptor();
    createPipeline();

    m_camera = new Camera();
    m_mvp.view = m_camera->getViewMatrix();
    m_mvp.proj = m_camera->getProjection( ( float )WIDTH / HEIGHT );
}

void App::createShader() {
    m_vertexShader = new Shader( m_device, "../shaders/spv/vert.spv", VK_SHADER_STAGE_VERTEX_BIT );
    m_pixelShader  = new Shader( m_device, "../shaders/spv/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT );
}

void App::createGeometry() {
    m_pCube = new Mesh( m_device, m_physicalDevice );
    m_pCube->createCube();
    m_pCube->cmdCreateVertexBuffer();
    m_pCube->cmdCreateIndexBuffer();

    m_pFloor = new Mesh( m_device, m_physicalDevice );
    m_pFloor->createPlane();
    m_pFloor->cmdCreateVertexBuffer();
    m_pFloor->cmdCreateIndexBuffer();
}

void App::createSwapchain() {
    VkSurfaceFormatKHR surfaceFormat = getSwapchainSurfaceFormat();
    VkPresentModeKHR   presentMode   = getSwapchainPresentMode();
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;
    
    m_extent = ChooseSwapExtent( capabilities, {WIDTH, HEIGHT} );
    
    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface          = m_surface;
    swapchainInfo.minImageCount    = imageCount;
    swapchainInfo.imageFormat      = surfaceFormat.format;
    swapchainInfo.imageColorSpace  = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent      = m_extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.preTransform     = capabilities.currentTransform;
    swapchainInfo.presentMode      = presentMode;
    swapchainInfo.clipped          = VK_TRUE;
    swapchainInfo.oldSwapchain     = VK_NULL_HANDLE;

    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if ( m_graphicQueueIndex != m_presentQueueIndex ) {
        uint32_t queueFamilyIndices[] = { m_graphicQueueIndex, m_presentQueueIndex };
        swapchainInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    
    VkResult result = vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain);
    CHECK_VKRESULT(result, "failed to create swap chain!");
    m_surfaceFormat = surfaceFormat.format;
}

void App::createRenderPass() {
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format          = m_surfaceFormat;
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format          = ChooseDepthFormat(m_physicalDevice);
    depthAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass    = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = UINT32(attachments.size());
    renderPassInfo.pAttachments     = attachments.data();
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &dependency;
    
    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    CHECK_VKRESULT(result, "failed to create render pass!");
}

void App::createFrameData() {
    std::vector<VkImage> swapchainImages = GetSwapchainImagesKHR( m_device, m_swapchain );
    m_totalFrame = UINT32( swapchainImages.size() );

    m_fb.resize( m_totalFrame ); 
    m_fbImages.resize( m_totalFrame );
    m_uniformBuffer.resize( m_totalFrame );
    m_imageSemaphores.resize( m_totalFrame );
    m_renderSemaphores.resize( m_totalFrame );
    m_commandFences.resize( m_totalFrame );
    m_cmdBuffers.resize( m_totalFrame );

    m_cmdBuffers = createCommandBuffers( m_totalFrame );

    m_depthImage = new Image(m_device, m_physicalDevice);
    m_depthImage->createForDepth( {WIDTH, HEIGHT}, 1 );

    for ( size_t i = 0; i < m_totalFrame; i++ ) {
        m_fbImages[i] = new Image( m_device, m_physicalDevice );
        m_fbImages[i]->createForSwapchain( swapchainImages[i], m_surfaceFormat );

        int attachmentCount = 2;
        VkImageView attachments[] = { m_fbImages[i]->getImageView(), m_depthImage->getImageView() };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = m_renderPass;
        framebufferInfo.attachmentCount = attachmentCount;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = WIDTH;
        framebufferInfo.height          = HEIGHT;
        framebufferInfo.layers          = 1;

        VkResult result = vkCreateFramebuffer( m_device, &framebufferInfo, nullptr, &m_fb[0] );
        CHECK_VKRESULT( result, "failed to create framebuffer!" );

        m_uniformBuffer[i] = new Buffer(m_device, m_physicalDevice);
        m_uniformBuffer[i]->setup( sizeof( UniformBuffer ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
        m_uniformBuffer[i]->create();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result = vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageSemaphores[i] );
        CHECK_VKRESULT( result, "failed to create image available semaphores!" );
        result = vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderSemaphores[i] );
        CHECK_VKRESULT( result, "failed to create semaphores!" );

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        result = vkCreateFence( m_device, &fenceInfo, nullptr, &m_commandFences[i] );
        CHECK_VKRESULT( result, "failed to create fences!" );
        
    }
}

void App::createDescriptor() {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    
    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descSetLayout);
    CHECK_VKRESULT(result, "failed to create descriptor set layout!");
    
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;

    result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descPool);
    CHECK_VKRESULT(result, "failed to create descriptor pool!");
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &m_descSetLayout;
    
    result = vkAllocateDescriptorSets(m_device, &allocInfo, &m_descSet);
    CHECK_VKRESULT(result, "failed to allocate descriptor set!");

    VkDescriptorBufferInfo bufferInfo = m_uniformBuffer[0]->getBufferInfo();
    m_writeDescSet.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    m_writeDescSet.dstBinding      = 0;
    m_writeDescSet.descriptorCount = 1;
    m_writeDescSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_writeDescSet.dstArrayElement = 0;
    m_writeDescSet.dstSet          = m_descSet;
    m_writeDescSet.pBufferInfo     = &bufferInfo;

    vkUpdateDescriptorSets( m_device, 1, &m_writeDescSet, 0, nullptr );
}

void App::createPipeline() {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = WIDTH;
    viewport.height = HEIGHT;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount  = 1;
    viewportInfo.pViewports    = &viewport;
    viewportInfo.pScissors     = &scissor;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.cullMode    = VK_CULL_MODE_NONE;
    rasterizationInfo.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.depthClampEnable        = VK_FALSE;
    rasterizationInfo.depthBiasEnable         = VK_FALSE;
    rasterizationInfo.lineWidth               = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable   = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable        = VK_TRUE;
    depthStencilInfo.depthWriteEnable       = VK_TRUE;
    depthStencilInfo.depthBoundsTestEnable  = VK_FALSE;
    depthStencilInfo.depthCompareOp         = VK_COMPARE_OP_LESS;
    depthStencilInfo.minDepthBounds         = 0.0f;
    depthStencilInfo.maxDepthBounds         = 1.0f;
    depthStencilInfo.stencilTestEnable      = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable     = VK_FALSE;
    colorBlendInfo.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount   = 1;
    colorBlendInfo.pAttachments      = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.push_back(m_vertexShader->getShaderStageInfo());
    shaderStages.push_back(m_pixelShader->getShaderStageInfo());
    
    VkPipelineVertexInputStateCreateInfo* vertexInputInfo = m_pCube->createVertexInputInfo();
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = &m_descSetLayout;
    
    VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    CHECK_VKRESULT(result, "failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout     = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass    = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = vertexInputInfo;
    pipelineInfo.pViewportState      = &viewportInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState   = &multisampleInfo;
    pipelineInfo.pColorBlendState    = &colorBlendInfo;
    pipelineInfo.pDepthStencilState  = &depthStencilInfo;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
    
    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    CHECK_VKRESULT(result, "failed to create graphics pipeline!");
}

void App::process() {
    while ( !glfwWindowShouldClose( m_window ) ) {
        glfwPollEvents();

        VkSemaphore     imageSemaphore  = m_imageSemaphores[m_currentFrame];
        VkCommandBuffer commandBuffer   = m_cmdBuffers[m_currentFrame];
        VkFence         commandFence    = m_commandFences[m_currentFrame];
        VkSemaphore     renderSemaphore = m_renderSemaphores[m_currentFrame];

        uint32_t imageIndex;
        VkResult result =  vkAcquireNextImageKHR(m_device, m_swapchain,
                                                 UINT64_MAX, imageSemaphore,
                                                 VK_NULL_HANDLE, &imageIndex);

        vkWaitForFences( m_device, 1, &commandFence, VK_TRUE, UINT64_MAX);
        
        {
            VkDeviceSize offsets[] = { 0 };
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color =  {0.1f, 0.1f, 0.1f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
    
            VkCommandBuffer commandBuffer = m_cmdBuffers[m_currentFrame];
            VkCommandBufferBeginInfo commandBeginInfo{};
            commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBeginInfo);
            CHECK_VKRESULT(result, "failed to begin recording command buffer!");
            {
                vkCmdSetLineWidth(commandBuffer, 1.0f);

                m_mvp.model = m_pCube->getMatrix();
                m_uniformBuffer[m_currentFrame]->fillBuffer( &m_mvp, sizeof( UniformBuffer ) );

        
                VkRenderPassBeginInfo renderBeginInfo{};
                renderBeginInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderBeginInfo.renderPass  = m_renderPass;
                renderBeginInfo.renderArea.extent = m_extent;
                renderBeginInfo.renderArea.offset = {0,0};
                renderBeginInfo.framebuffer     = m_fb[m_currentFrame];
                renderBeginInfo.clearValueCount = UINT32(clearValues.size());
                renderBeginInfo.pClearValues    = clearValues.data();
                vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
                {
                    VkBuffer vertexBuffers[] = {m_pCube->m_vertexBuffer->m_buffer};
                    VkBuffer indexBuffers    = m_pCube->m_indexBuffer->m_buffer;
                    uint32_t indexSize       = UINT32( m_pCube->m_indices.size());
            
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
                
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            m_pipelineLayout, 0, 1, &m_descSet, 0, nullptr);
                
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer  (commandBuffer, indexBuffers, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdDrawIndexed(commandBuffer, indexSize, 1, 0, 0, 0);
                }
                {
                    VkBuffer vertexBuffers[] = {m_pFloor->m_vertexBuffer->m_buffer};
                    VkBuffer indexBuffers    = m_pFloor->m_indexBuffer->m_buffer;
                    uint32_t indexSize       = UINT32(m_pFloor->m_indices.size());
            
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
                
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            m_pipelineLayout, 0, 1, &m_descSet, 0, nullptr);
                
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer  (commandBuffer, indexBuffers, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdDrawIndexed(commandBuffer, indexSize, 1, 0, 0, 0);
                }

                vkCmdEndRenderPass(commandBuffer);
            }
            result = vkEndCommandBuffer(commandBuffer);


        }

        VkSemaphore waitSemaphore[]   = { imageSemaphore };
        VkSemaphore signalSemaphors[] = { renderSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphors;
    
        vkResetFences(m_device, 1, &commandFence);
        result = vkQueueSubmit(m_graphicQueue, 1, &submitInfo, commandFence);
        CHECK_VKRESULT(result, "failed to submit draw command buffer!");

    
        VkSwapchainKHR swapchains[] = { m_swapchain };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = swapchains;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphors;
        presentInfo.pImageIndices      = &imageIndex;

        result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    }
}
