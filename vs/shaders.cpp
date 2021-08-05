#include "app.h"
#include "helper.h"

void App::settingShader() {
    {
        auto code = ReadBinaryFile( m_vertexShaderPath );

        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = code.size();
        shaderInfo.pCode = reinterpret_cast< const uint32_t* >( code.data() );

        VkResult result = vkCreateShaderModule( m_device, &shaderInfo, nullptr, &m_shaderModuleVertex );
        CHECK_VKRESULT( result, "failed to create shader modul!" );

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageInfo.pName = "main";
        shaderStageInfo.module = m_shaderModuleVertex;
    }
    {
        auto code = ReadBinaryFile( m_pixelShaderPath );

        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = code.size();
        shaderInfo.pCode = reinterpret_cast< const uint32_t* >( code.data() );

        VkResult result = vkCreateShaderModule( m_device, &shaderInfo, nullptr, &m_shaderModulePixel );
        CHECK_VKRESULT( result, "failed to create shader modul!" );

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageInfo.pName = "main";
        shaderStageInfo.module = m_shaderModulePixel;
    }


}