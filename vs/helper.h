//  Copyright © 2020 Subph. All rights reserved.
//

#pragma once

#include "common.h"

std::vector<VkImage> GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain);

int32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, int32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat   ChooseDepthFormat(VkPhysicalDevice physicalDevice);
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Size<int> size);

std::vector<char> ReadBinaryFile (const std::string filename);
