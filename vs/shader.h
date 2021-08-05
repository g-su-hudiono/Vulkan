#pragma once

#include "common.h"

class Shader {
    
public:
    ~Shader();
    Shader();
    Shader(VkDevice device, const std::string filepath, VkShaderStageFlagBits stage, const char* entryPoint = "main");
    
    void cleanup();
    VkPipelineShaderStageCreateInfo getShaderStageInfo();
    
private:
    
    VkDevice       m_device       = VK_NULL_HANDLE;
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    
    VkShaderModuleCreateInfo        m_shaderInfo{};
    VkPipelineShaderStageCreateInfo m_shaderStageInfo{};
    
    std::string m_filepath;
    
};
