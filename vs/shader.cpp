#include "shader.h"
#include "helper.h"

Shader::~Shader() {}
Shader::Shader() {}

Shader::Shader(VkDevice device, const std::string filepath, VkShaderStageFlagBits stage, const char* entryPoint) {
    m_device = device;
    m_filepath = filepath;
    auto code = ReadBinaryFile(filepath);
    
    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = code.size();
    shaderInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    
    VkResult result = vkCreateShaderModule( m_device, &shaderInfo, nullptr, &m_shaderModule );
    CHECK_VKRESULT(result, "failed to create shader modul!");
    
    m_shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_shaderStageInfo.stage  = stage;
    m_shaderStageInfo.pName  = entryPoint;
    m_shaderStageInfo.module = m_shaderModule;
}

void Shader::cleanup() {
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo Shader::getShaderStageInfo() {
    return m_shaderStageInfo;
}
