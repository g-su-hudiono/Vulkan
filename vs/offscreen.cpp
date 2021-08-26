#include "app.h"
#include "helper.h"

void App::createOffscreenRenderPass() {
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
    colorAttachment.format          = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_GENERAL;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_GENERAL;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format          = ChooseDepthFormat(m_physicalDevice);
    depthAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
    
    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_offscreenRenderPass );
    CHECK_VKRESULT(result, "failed to create render pass!");
}

void App::createOffscreenFramedata() {
    m_offscreenImage = new Image( m_device, m_physicalDevice );
    m_offscreenImage->createForOffscreen( { WIDTH, HEIGHT } );

    m_offscreenDepth = new Image( m_device, m_physicalDevice );
    m_offscreenDepth->createForDepth( { WIDTH, HEIGHT } );

    int attachmentCount = 2;
    VkImageView attachments[] = { m_offscreenImage->getImageView(), m_offscreenDepth->getImageView() };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = m_offscreenRenderPass;
    framebufferInfo.attachmentCount = attachmentCount;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = WIDTH;
    framebufferInfo.height          = HEIGHT;
    framebufferInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer( m_device, &framebufferInfo, nullptr, &m_offscreenFramebuffer );
    CHECK_VKRESULT( result, "failed to create framebuffer!" );
}

void App::createGraphicsPipeline() {
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

    Shader* vertexShader = new Shader( m_device, "../shaders/spv/vert.spv", VK_SHADER_STAGE_VERTEX_BIT );
    Shader* pixelShader = new Shader( m_device, "../shaders/spv/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT );
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.push_back(vertexShader->getShaderStageInfo());
    shaderStages.push_back(pixelShader->getShaderStageInfo());
    
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
    pipelineInfo.renderPass = m_offscreenRenderPass;
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
