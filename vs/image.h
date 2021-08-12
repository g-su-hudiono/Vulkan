//  Copyright © 2021 Subph. All rights reserved.
//

#pragma once

#include "common.h"

class Renderer;

class Image {
    
public:
    ~Image();
    Image( VkDevice device, VkPhysicalDevice physicalDevice );

    void cleanup();
    void cleanupImageView();
    
    void createForDepth     (Size<int32_t> size);
    void createForSwapchain (VkImage image, VkFormat imageFormat);
    void createForOffscreen (Size<int32_t> size);
    void allocateImageMemory();
    void createSampler      ();
    
    VkImage          getImage      ();
    VkImageView      getImageView  ();
    VkDeviceMemory   getImageMemory();
    
private:
        
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    VkImage          m_image          = VK_NULL_HANDLE;
    VkImageView      m_imageView      = VK_NULL_HANDLE;
    VkDeviceMemory   m_imageMemory    = VK_NULL_HANDLE;
    VkSampler        m_sampler        = VK_NULL_HANDLE;

    static VkFormat ChooseDepthFormat( VkPhysicalDevice physicalDevice );
    static VkImageCreateInfo     GetDefaultImageCreateInfo();
    static VkImageViewCreateInfo GetDefaultImageViewCreateInfo();
    static VkImageMemoryBarrier  GetDefaultImageMemoryBarrier();

    
};
