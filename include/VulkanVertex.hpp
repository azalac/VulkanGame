/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanVertex.hpp
 * Author: austin-z
 *
 * Created on February 12, 2019, 8:08 PM
 */

#ifndef VULKANVERTEX_HPP
#define VULKANVERTEX_HPP

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include <vector>
#include <array>
#include <glm/glm.hpp>

// A helper class for the various glm vector to vulkan vector formats
class GLMVectorFormats {
public:
    static constexpr vk::Format
    VEC1 = vk::Format::eR32Sfloat,
            VEC2 = vk::Format::eR32G32Sfloat,
            VEC3 = vk::Format::eR32G32B32Sfloat,
            VEC4 = vk::Format::eR32G32B32A32Sfloat;
};

// Represents a vertex descriptor and binding
class VulkanVertexDescriptor {
protected:

    vk::VertexInputBindingDescription binding;

public:

    VulkanVertexDescriptor(int binding, size_t stride,
            vk::VertexInputRate rate = vk::VertexInputRate::eVertex) {
        this->binding.binding = binding;
        this->binding.inputRate = rate;
        this->binding.stride = stride;
    }

    vk::VertexInputBindingDescription getBinding(void) const {
        return binding;
    }

    virtual std::vector<vk::VertexInputAttributeDescription> getAttributes(void) = 0;
};

// Controls the pipeline vertex input state
class VulkanVertexInputState {
public:

    vk::PipelineVertexInputStateCreateFlags flags;

    std::vector<VulkanVertexDescriptor*> descriptors;

    std::vector<vk::VertexInputBindingDescription> vertexInputBinding;
    std::vector<vk::VertexInputAttributeDescription> vertexAttribute;

    VulkanVertexInputState(vk::PipelineVertexInputStateCreateFlags flags = vk::PipelineVertexInputStateCreateFlags()) {
        this->flags = flags;
    }

    void addDescriptor(VulkanVertexDescriptor * descriptor) {
        descriptors.push_back(descriptor);
        vertexInputBinding.push_back(descriptor->getBinding());

        for (auto & attr : descriptor->getAttributes()) {
            vertexAttribute.push_back(attr);
        }
    }

    operator vk::PipelineVertexInputStateCreateInfo() {
        return vk::PipelineVertexInputStateCreateInfo(flags,
                static_cast<uint32_t> (vertexInputBinding.size()), vertexInputBinding.data(),
                static_cast<uint32_t> (vertexAttribute.size()), vertexAttribute.data());
    }

};

// Represents a single vertex
struct VulkanVertexObject {
    glm::vec2 position;
    glm::vec3 color;
    glm::vec2 uv;

    static std::vector<vk::VertexInputAttributeDescription> describe(int binding) {

        std::array<int, 3> locations = {0, 1, 2};
        std::array<vk::Format, 3> formats = {GLMVectorFormats::VEC2, GLMVectorFormats::VEC3, GLMVectorFormats::VEC2};
        std::array<size_t, 3> offsets = {offsetof(VulkanVertexObject, position), offsetof(VulkanVertexObject, color), offsetof(VulkanVertexObject, uv)};

        std::vector<vk::VertexInputAttributeDescription> descriptions;

        for (size_t i = 0; i < locations.size(); i++) {
            descriptions.push_back(vk::VertexInputAttributeDescription(locations[i], binding, formats[i], offsets[i]));
        }

        return descriptions;
    }
};

// Represents a Vertex Buffer of VulkanVertexObjects
template<class T>
class VulkanVertexBuffer : public VulkanObjectBuffer<T>, public VulkanVertexDescriptor {
public:

    VulkanVertexBuffer(VulkanDevice * device, int binding, int nverts, vk::VertexInputRate rate = vk::VertexInputRate::eVertex) :
    VulkanObjectBuffer<T>(device, nverts, vk::BufferUsageFlagBits::eVertexBuffer),
    VulkanVertexDescriptor(binding, sizeof (T), rate) {

    }

    std::vector<vk::VertexInputAttributeDescription> getAttributes(void) {
        return T::describe(this->binding.binding);
    }

};

// Represents an index buffer
class VulkanIndexBuffer : public VulkanObjectBuffer<uint16_t> {
public:

    VulkanIndexBuffer(VulkanDevice * device, size_t size, std::vector<uint16_t> initial = std::vector<uint16_t>()) :
    VulkanObjectBuffer(device, size, vk::BufferUsageFlagBits::eIndexBuffer) {

        for (size_t i = 0; i < size && i < initial.size(); i++) {
            (*this)[i] = initial[i];
        }
    }

};

using VulkanVertexBufferDefault = VulkanVertexBuffer<VulkanVertexObject>;

#endif /* VULKANVERTEX_HPP */

