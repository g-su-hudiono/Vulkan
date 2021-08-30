//  Copyright © 2021 Subph. All rights reserved.
//

#include "image.h"

#include "helper.h"
#include "buffer.h"

Image::~Image() {}
Image::Image( VkDevice device, VkPhysicalDevice physicalDevice ) :
    m_device( device ),
    m_physicalDevice( physicalDevice ) {}

void Image::cleanup() {
    LOG("Image::cleanup");
    cleanupImageView();
    if (m_image == VK_NULL_HANDLE) return;
    vkDestroyImage(m_device, m_image, nullptr);
}

void Image::cleanupImageView() {
    LOG("Image::cleanupImageView");
    vkDestroyImageView(m_device, m_imageView  , nullptr);
    vkFreeMemory      (m_device, m_imageMemory, nullptr);
}

void Image::createForSwapchain(VkImage image, VkFormat imageFormat) {
    m_image = image;
    VkImageViewCreateInfo imageViewInfo = GetDefaultImageViewCreateInfo();

    imageViewInfo.format = imageFormat;
    imageViewInfo.image  = image;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    
    VkResult result = vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_imageView);
    CHECK_VKRESULT(result, "failed to create image views!");
}

void Image::createForDepth(Size<int32_t> size) {
    VkFormat        depthFormat = ChooseDepthFormat( m_physicalDevice );
    VkImageCreateInfo imageInfo = GetDefaultImageCreateInfo();
    imageInfo.extent.width  = size.width;
    imageInfo.extent.height = size.height;
    imageInfo.mipLevels     = 1;
    imageInfo.format        = depthFormat;
    imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
    CHECK_VKRESULT(result, "failed to create image!");

    allocateImageMemory();

    VkImageViewCreateInfo imageViewInfo = GetDefaultImageViewCreateInfo();
    imageViewInfo.image  = m_image;
    imageViewInfo.format = depthFormat;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    result = vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_imageView);
    CHECK_VKRESULT(result, "failed to create image views!");
}

void Image::createForOffscreen(Size<int32_t> size) {
    VkImageCreateInfo imageInfo = GetDefaultImageCreateInfo();
    imageInfo.extent.width  = size.width;
    imageInfo.extent.height = size.height;
    imageInfo.mipLevels     = 1;
    imageInfo.format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT;
    
    VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
    CHECK_VKRESULT(result, "failed to create image!");

    allocateImageMemory();

    VkImageViewCreateInfo imageViewInfo = GetDefaultImageViewCreateInfo();
    imageViewInfo.image  = m_image;
    imageViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    result = vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_imageView);
    CHECK_VKRESULT(result, "failed to create image views!");

    createSampler();
}

void Image::allocateImageMemory() {
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements( m_device, m_image, &memoryRequirements);
    
    int32_t memoryTypeIndex;
    memoryTypeIndex = FindMemoryTypeIndex( m_physicalDevice,
                                           memoryRequirements.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memoryRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    VkDeviceMemory imageMemory;
    VkResult       result = vkAllocateMemory( m_device, &allocInfo, nullptr, &imageMemory);
    CHECK_VKRESULT(result, "failed to allocate image memory!");
    
    vkBindImageMemory( m_device, m_image, imageMemory, 0);
    
    m_imageMemory = imageMemory;
}

void Image::createSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = VK_FILTER_LINEAR;
    samplerInfo.minFilter    = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy    = 16.0f;
    samplerInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable    = VK_FALSE;
    samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias       = 0.0f;
    samplerInfo.minLod           = 0.0f;
    samplerInfo.maxLod           = 1;
    
    VkResult  result = vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler );
    CHECK_VKRESULT(result, "failed to create texture sampler!");
}

VkImage         Image::getImage      () { return m_image;       }
VkImageView     Image::getImageView  () { return m_imageView;   }
VkDeviceMemory  Image::getImageMemory() { return m_imageMemory; }

VkDescriptorImageInfo Image::getDescriptor()
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView   = m_imageView;
    imageInfo.sampler     = m_sampler;
    return imageInfo;
}


// Private ==================================================


VkFormat Image::ChooseDepthFormat(VkPhysicalDevice physicalDevice) {
    const std::vector<VkFormat>& candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    
    throw std::runtime_error("failed to find depth format!");
}

VkImageCreateInfo Image::GetDefaultImageCreateInfo() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.arrayLayers   = 1;
    imageInfo.extent.depth  = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    return imageInfo;
}

VkImageViewCreateInfo Image::GetDefaultImageViewCreateInfo() {
    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.subresourceRange.layerCount     = 1;
    imageViewInfo.subresourceRange.baseMipLevel   = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    return imageViewInfo;
}

