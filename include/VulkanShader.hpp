#pragma once

#include <string>
#include <stdio.h>
#include <vector>

#include "VulkanDevice.hpp"

class VulkanShader {
private:

    std::vector<uint32_t> data;

    vk::ShaderModule shader;

    std::string name;

    vk::ShaderStageFlagBits type;

public:

    VulkanShader(VulkanDevice & device, const std::string & filename,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    VulkanShader(VulkanDevice & device, std::vector<uint32_t> data,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    VulkanShader(VulkanDevice & device, const uint32_t * data, size_t count,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    operator vk::PipelineShaderStageCreateInfo() {
        return vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), type, shader, name.c_str());
    }

    void dumpBytecode(int words_per_row = 8);

private:

    static std::vector<uint32_t> LoadShader(const std::string & filename);

    void init(VulkanDevice & device, const uint32_t * data, size_t count) {

        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = count;
        createInfo.pCode = data;

        shader = device->createShaderModule(createInfo);
    }

};
