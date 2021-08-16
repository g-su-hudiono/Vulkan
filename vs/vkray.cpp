#include "app.h"


void App::initRayTracing() {
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
    VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    properties2.pNext = &rtProperties;
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


    VkAccelerationStructureKHR accelStructure{};

    PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructureKHR = 
        (PFN_vkCreateAccelerationStructureKHR) vkGetInstanceProcAddr( m_instance, "vkCreateAccelerationStructureKHR" );
    CreateAccelerationStructureKHR( m_device, &createInfo, nullptr, &accelStructure );

    //vkCreateAccelerationStructureKHR( m_device, &createInfo, nullptr, &accelStructure );

    Buffer* accelStructureBuffer = new Buffer( m_device, m_physicalDevice );
    accelStructureBuffer->setup( createInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT );
    accelStructureBuffer->create();
    buildInfo.dstAccelerationStructure = accelStructure;


    Buffer* scratchBuffer = new Buffer( m_device, m_physicalDevice );
    scratchBuffer->setup( sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT );

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
    buildInfo.scratchData.deviceAddress = scratchAddress;
    const VkAccelerationStructureBuildRangeInfoKHR* accelRange = &offset;

    PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructuresKHR =
        ( PFN_vkCmdBuildAccelerationStructuresKHR )vkGetInstanceProcAddr( m_instance, "vkCmdBuildAccelerationStructuresKHR" );
    CmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildInfo, &accelRange );

    //vkCmdBuildAccelerationStructuresKHR( cmdBuffer, 1, &buildInfo, &accelRange );

    // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
    // is finished before starting the next one
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier( cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    endSingleTimeCommands( cmdBuffer );
}

void App::createTopLevelAS() {
}

void App::createRtDescriptorSet() {
}

void App::createRtPipeline() {

}

void App::createRtShaderBindingTable() {

}