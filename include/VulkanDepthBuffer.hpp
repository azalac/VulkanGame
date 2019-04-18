#pragma once

#include "VulkanDevice.hpp"

// Controls the depth buffer - not currently used but will be in the future
class VulkanDepthBuffer {
private:

    vk::UniqueImage buffer;

    vk::UniqueDeviceMemory depthMemory;
    vk::UniqueImageView depthView;

    vk::Format depthFormat;

public:

    VulkanDepthBuffer(VulkanDevice & device, VulkanViewport & viewport, const vk::Format depthFormat = vk::Format::eD16Unorm);

    const vk::Format getBufferFormat(void) const;

private:

    vk::ImageTiling getImageTiling(VulkanDevice & device, const vk::Format depthFormat);

    void createImage(VulkanDevice & device, uint32_t width, uint32_t height, vk::Format depthFormat, vk::ImageTiling tiling);

    void createImageMemory(VulkanDevice & device, vk::MemoryRequirements & memoryRequirements, uint32_t typeIndex, vk::Format depthFormat);

    // Copied from the vulkan specification, section 10.2, with slight modifications

    int32_t findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties & MemoryProperties, uint32_t memoryTypeBitsRequirement, vk::MemoryPropertyFlags requiredProperties);
};
