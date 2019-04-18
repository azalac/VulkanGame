#pragma once

#include <string>
#include <stdio.h>
#include <vector>

#include "VulkanDevice.hpp"

// Represents a single shader module
// Can load from multiple sources
class VulkanShader {
private:

    vk::ShaderModule shader;

    std::string name;

    vk::ShaderStageFlagBits type;

    std::vector<uint32_t> data;

public:

    /**
     * Loads a shader from compiled SPIR-V bytecode
     * @param device The device
     * @param filename The file
     * @param type The shader stage
     * @param shader_name The shader entry point
     */
    VulkanShader(VulkanDevice & device, const std::string & filename,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    /**
     * Loads a shader from a SPIR-v bytecode vector
     * @param device The device
     * @param data The bytecode vector
     * @param type The shader stage
     * @param shader_name The shader entry point
     */
    VulkanShader(VulkanDevice & device, std::vector<uint32_t> data,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    /**
     * Loads a shader from a SPIR-v bytecode array
     * @param device The device
     * @param data The bytecode buffer
     * @param count The buffer size in bytes
     * @param type The shader stage
     * @param shader_name The shader entry point
     */
    VulkanShader(VulkanDevice & device, const uint32_t * data, size_t count,
            vk::ShaderStageFlagBits type = vk::ShaderStageFlagBits::eVertex,
            const std::string shader_name = "main");

    operator vk::PipelineShaderStageCreateInfo() {
        return vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), type, shader, name.c_str());
    }

    /**
     * Dumps this shader's bytecode to stdout
     * @param words_per_row The number of 4-byte words per row
     */
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
