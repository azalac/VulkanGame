/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanShader.hpp"

#include <inttypes.h>
#include <fstream>

VulkanShader::VulkanShader(VulkanDevice & device, const std::string & filename,
        vk::ShaderStageFlagBits type, const std::string shader_name) :
name(shader_name), type(type), data(LoadShader(filename)) {

    init(device, data.data(), data.size() * sizeof(uint32_t));
}

VulkanShader::VulkanShader(VulkanDevice & device, std::vector<uint32_t> data,
        vk::ShaderStageFlagBits type, const std::string shader_name) :
name(shader_name), type(type) {

    this->data = data;
    init(device, data.data(), data.size() * sizeof(uint32_t));
}

VulkanShader::VulkanShader(VulkanDevice & device, const uint32_t * data, size_t count,
        vk::ShaderStageFlagBits type, const std::string shader_name) :
name(shader_name), type(type) {

    this->data.resize(count / sizeof(uint32_t));

	memcpy(this->data.data(), data, count);

    init(device, data, count);
}

void VulkanShader::dumpBytecode(int words_per_row) {
    printf("bytecode dump:");

    for (size_t i = 0; i < data.size(); i++) {
        if (i % words_per_row == 0) {
            printf("\n%2d\t", (int) i);
        }

        printf("%08X, ", data[i]);
    }

    printf("\n");
}

std::vector<uint32_t> VulkanShader::LoadShader(const std::string & filename) {
    std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Could not find file " + filename);
    }

    size_t size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);

    char * buffer = new char[size];
    
    if (!file.read(buffer, size)) {
        delete[] buffer;
        throw std::runtime_error("Could not load file");
    }

    file.close();

    uint32_t * ptr = reinterpret_cast<uint32_t*>(buffer);
    std::vector<uint32_t> buffer2(size / sizeof(uint32_t));
    
    for(size_t i = 0; i < buffer2.size(); i ++) {
        buffer2[i] = ptr[i];
    }
    
    delete[] buffer;
    
    return buffer2;
}
