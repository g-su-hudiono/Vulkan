#include "app.h"


void App::initRayTracing() {
    m_rtProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
    VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    properties2.pNext = &m_rtProperties;
    vkGetPhysicalDeviceProperties2( m_physicalDevice, &properties2 );
}

void App::createBottomLevelAS() {
    VkBuffer vertexBuffer = m_pCube->m_vertexBuffer->getBuffer();
    VkBufferDeviceAddressInfo vertexInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    vertexInfo.buffer = vertexBuffer;
    VkDeviceAddress vertexAddress = vkGetBufferDeviceAddress( m_device, &vertexInfo );

    VkBuffer indexBuffer = m_pCube->m_indexBuffer->getBuffer();
    VkBufferDeviceAddressInfo indexInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    indexInfo.buffer = indexBuffer;

    VkDeviceAddress indexAddress = vkGetBufferDeviceAddress( m_device, &indexInfo );

    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = vertexAddress;
    triangles.vertexStride             = 1;
    triangles.indexType                = VK_INDEX_TYPE_UINT32;
    triangles.indexData.deviceAddress  = indexAddress;
    triangles.maxVertex                = static_cast<uint32_t>(m_pCube->m_positions.size());

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles = triangles;

    VkAccelerationStructureBuildRangeInfoKHR offset{};
    offset.firstVertex     = 0;
    offset.primitiveCount  = static_cast< uint32_t >( m_pCube->m_indices.size() / 3 );
    offset.primitiveOffset = 0;
    offset.transformOffset = 0;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geometry;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    VkDeviceSize              maxScratch{ 0 };
    std::vector<VkDeviceSize> originalSizes( 1 );

    uint32_t maxPrimCount = offset.primitiveCount;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes = 
        (PFN_vkGetAccelerationStructureBuildSizesKHR) vkGetInstanceProcAddr( m_instance, "vkGetAccelerationStructureBuildSizesKHR" );
    GetAccelerationStructureBuildSizes( m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
        &maxPrimCount, &sizeInfo );

    //vkGetAccelerationStructureBuildSizesKHR( m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
    //    &maxPrimCount, &sizeInfo );

    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    createInfo.size = sizeInfo.accelerationStructureSize;

    Buffer* accelStructureBuffer = new Buffer( m_device, m_physicalDevice );
    accelStructureBuffer->setup( createInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT );
    accelStructureBuffer->create();
    createInfo.buffer = accelStructureBuffer->getBuffer();

    VkAccelerationStructureKHR accelStructure;

    PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructureKHR = 
        (PFN_vkCreateAccelerationStructureKHR) vkGetInstanceProcAddr( m_instance, "vkCreateAccelerationStructureKHR" );
    CreateAccelerationStructureKHR( m_device, &createInfo, nullptr, &accelStructure );

    //vkCreateAccelerationStructureKHR( m_device, &createInfo, nullptr, &accelStructure );

    buildInfo.dstAccelerationStructure = accelStructure;


    Buffer* scratchBuffer = new Buffer( m_device, m_physicalDevice );
    scratchBuffer->setup( sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT );
    scratchBuffer->create();

    VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    bufferInfo.buffer = scratchBuffer->m_buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress( m_device, &bufferInfo );
    buildInfo.scratchData.deviceAddress = scratchAddress;
    
    // Allocate a query pool for storing the needed size for every BLAS compaction.
    VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qpci.queryCount = 1;
    qpci.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool;
    vkCreateQueryPool(m_device, &qpci, nullptr, &queryPool);
    vkResetQueryPool(m_device, queryPool, 0, 1);

    VkCommandBuffer cmdBuffer = beginSingleTimeCommands();
    const VkAccelerationStructureBuildRangeInfoKHR* accelRange = &offset;

    PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructuresKHR =
        ( PFN_vkCmdBuildAccelerationStructuresKHR )vkGetInstanceProcAddr( m_instance, "vkCmdBuildAccelerationStructuresKHR" );
    CmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildInfo, &accelRange );

    //vkCmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildInfo, &accelRange );

    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier( cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    endSingleTimeCommands( cmdBuffer );

    m_blAccelStructure = accelStructure;
}

void App::createTopLevelAS() {
    VkAccelerationStructureInstanceKHR geometryInstance;
    
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = m_blAccelStructure;
    //VkDeviceAddress blasAddress       = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addressInfo);

    PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddress =
        ( PFN_vkGetAccelerationStructureDeviceAddressKHR )vkGetInstanceProcAddr( m_instance, "vkGetAccelerationStructureDeviceAddressKHR" );
    VkDeviceAddress blasAddress = GetAccelerationStructureDeviceAddress( m_device, &addressInfo );

    const VkTransformMatrixKHR transform = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };
    geometryInstance.transform                              = transform;
    geometryInstance.instanceCustomIndex                    = 0;
    geometryInstance.mask                                   = 0xFF;
    geometryInstance.instanceShaderBindingTableRecordOffset = 0;
    geometryInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    geometryInstance.accelerationStructureReference         = blasAddress;

    VkDeviceSize instanceSize = sizeof( VkAccelerationStructureInstanceKHR );

    Buffer* instanceBuffer = new Buffer( m_device, m_physicalDevice );
    instanceBuffer->setup( instanceSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
    instanceBuffer->create();
    instanceBuffer->fillBuffer( &geometryInstance, instanceSize );

    VkCommandBuffer cmdBuffer = beginSingleTimeCommands();
    
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer               = instanceBuffer->getBuffer();
    VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);

    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier( cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                        0, 1, &barrier, 0, nullptr, 0, nullptr);


    VkAccelerationStructureGeometryInstancesDataKHR instancesVk{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instancesVk.arrayOfPointers    = VK_FALSE;
    instancesVk.data.deviceAddress = instanceAddress;

    VkAccelerationStructureGeometryKHR topASGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    topASGeometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.geometry.instances = instancesVk;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &topASGeometry;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    uint32_t count = 1;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

    PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes =
        ( PFN_vkGetAccelerationStructureBuildSizesKHR )vkGetInstanceProcAddr( m_instance, "vkGetAccelerationStructureBuildSizesKHR" );
    GetAccelerationStructureBuildSizes( m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
        &count, &sizeInfo );

    VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    createInfo.size = sizeInfo.accelerationStructureSize;
    
    Buffer* accelStructureBuffer = new Buffer( m_device, m_physicalDevice );
    accelStructureBuffer->setup( createInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT );
    accelStructureBuffer->create();
    createInfo.buffer = accelStructureBuffer->getBuffer();

    VkAccelerationStructureKHR accelStructure;

    PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructureKHR = 
        (PFN_vkCreateAccelerationStructureKHR) vkGetInstanceProcAddr( m_instance, "vkCreateAccelerationStructureKHR" );
    CreateAccelerationStructureKHR( m_device, &createInfo, nullptr, &accelStructure );

    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = accelStructure;


    Buffer* scratchBuffer = new Buffer( m_device, m_physicalDevice );
    scratchBuffer->setup( sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR );
    scratchBuffer->create();

    VkBufferDeviceAddressInfo bufferInfoScratch{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    bufferInfoScratch.buffer = scratchBuffer->m_buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress( m_device, &bufferInfoScratch );
    buildInfo.scratchData.deviceAddress = scratchAddress;

    
    VkAccelerationStructureBuildRangeInfoKHR        offset{static_cast<uint32_t>(1), 0, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR* accelRange = &offset;

    // Build the TLAS
    PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructuresKHR =
        ( PFN_vkCmdBuildAccelerationStructuresKHR )vkGetInstanceProcAddr( m_instance, "vkCmdBuildAccelerationStructuresKHR" );
    CmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildInfo, &accelRange );

    endSingleTimeCommands( cmdBuffer );
    m_tlAccelStructure = accelStructure;
}

void App::createRtDescriptorSet() {
    VkDescriptorSetLayoutBinding layoutBinding0{};
    layoutBinding0.binding         = 0;
    layoutBinding0.descriptorCount = 1;
    layoutBinding0.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    layoutBinding0.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    layoutBinding0.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutBinding layoutBinding1{};
    layoutBinding1.binding         = 1;
    layoutBinding1.descriptorCount = 1;
    layoutBinding1.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBinding1.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    layoutBinding1.pImmutableSamplers = nullptr;
    
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { layoutBinding0, layoutBinding1};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings    = layoutBindings.data();
    
    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_rtDescSetLayout);
    CHECK_VKRESULT(result, "failed to create descriptor set layout!");
    
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    
    result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_rtDescPool);
    CHECK_VKRESULT(result, "failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool     = m_rtDescPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts        = &m_rtDescSetLayout;
    vkAllocateDescriptorSets(m_device, &allocateInfo, &m_rtDescSet );


    VkAccelerationStructureKHR tlas = m_tlAccelStructure;
    VkWriteDescriptorSetAccelerationStructureKHR descASInfo{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    descASInfo.accelerationStructureCount = 1;
    descASInfo.pAccelerationStructures    = &tlas;
    VkDescriptorImageInfo imageInfo{{}, m_offscreenImage->getImageView(), VK_IMAGE_LAYOUT_GENERAL};
    
    VkWriteDescriptorSet writeSet0{};
    writeSet0.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet0.dstBinding      = 0;
    writeSet0.descriptorCount = 1;
    writeSet0.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writeSet0.dstArrayElement = 0;
    writeSet0.dstSet          = m_rtDescSet;
    writeSet0.pNext           = &descASInfo;
    
    VkWriteDescriptorSet writeSet1{};
    writeSet1.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet1.dstBinding      = 1;
    writeSet1.descriptorCount = 1;
    writeSet1.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSet1.dstArrayElement = 0;
    writeSet1.dstSet          = m_rtDescSet;
    writeSet1.pImageInfo      = &imageInfo;

    std::vector<VkWriteDescriptorSet> writes = { writeSet0, writeSet1 };
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

}

void App::createRtPipeline() {
    Shader* rayGenShader  = new Shader( m_device, "../shaders/spv/raytrace.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR );
    Shader* rayMissShader = new Shader( m_device, "../shaders/spv/raytrace.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR );
    Shader* rayShadowShader = new Shader( m_device, "../shaders/spv/raytraceShadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR );
    Shader* rayHitShader  = new Shader( m_device, "../shaders/spv/raytrace.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR );

    std::array<VkPipelineShaderStageCreateInfo, 3> stages{};
    stages[0] = rayGenShader->getShaderStageInfo();
    stages[1] = rayMissShader->getShaderStageInfo();
    stages[2] = rayHitShader->getShaderStageInfo();

    VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    group.generalShader      = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = 0;
    m_rtShaderGroups.push_back( group );

    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = 1;
    m_rtShaderGroups.push_back( group );

    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = 2;
    m_rtShaderGroups.push_back( group );

    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = 3;
    m_rtShaderGroups.push_back( group );

    VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                                     0, sizeof( RtPushConstant ) };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

    std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { m_rtDescSetLayout, m_descSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = static_cast< uint32_t >( rtDescSetLayouts.size() );
    pipelineLayoutCreateInfo.pSetLayouts    = rtDescSetLayouts.data();

    vkCreatePipelineLayout( m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout );

    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    rayPipelineInfo.stageCount = static_cast< uint32_t >( stages.size() ); 
    rayPipelineInfo.pStages    = stages.data();
    rayPipelineInfo.groupCount = static_cast< uint32_t >( m_rtShaderGroups.size() );
    rayPipelineInfo.pGroups    = m_rtShaderGroups.data();
    rayPipelineInfo.layout     = m_rtPipelineLayout;
    rayPipelineInfo.maxPipelineRayRecursionDepth = 2;

    // vkCreateRayTracingPipelinesKHR( m_device, {}, {}, 1, & rayPipelineInfo, nullptr, & m_rtPipeline );

    PFN_vkCreateRayTracingPipelinesKHR CreateRayTracingPipelinesKHR =
        ( PFN_vkCreateRayTracingPipelinesKHR )vkGetInstanceProcAddr( m_instance, "vkCreateRayTracingPipelinesKHR" );
    CreateRayTracingPipelinesKHR( m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline );
}

void App::createRtShaderBindingTable() {

}