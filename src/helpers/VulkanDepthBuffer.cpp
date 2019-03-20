/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanDepthBuffer.hpp"

VulkanDepthBuffer::VulkanDepthBuffer(VulkanDevice & device, VulkanViewport & viewport, const vk::Format depthFormat) {

    this->depthFormat = depthFormat;

    vk::ImageTiling tiling = getImageTiling(device, depthFormat);

    createImage(device, viewport.getWidth(), viewport.getHeight(), depthFormat, tiling);

    vk::MemoryRequirements memoryRequirements = device->getImageMemoryRequirements(buffer.get());
    vk::PhysicalDeviceMemoryProperties memoryProperties = device.getMemoryProperties();

    uint32_t typeIndex = findMemoryTypeIndex(memoryProperties, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    if (typeIndex == (uint32_t)-1) {
        throw std::runtime_error("Can not find device local memory");
    }

    createImageMemory(device, memoryRequirements, typeIndex, depthFormat);

}

const vk::Format VulkanDepthBuffer::getBufferFormat(void) const {
    return depthFormat;
}

vk::ImageTiling VulkanDepthBuffer::getImageTiling(VulkanDevice & device, const vk::Format depthFormat) {

    vk::FormatProperties formatProperties = device.getFormatProperties(depthFormat);

    if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        return vk::ImageTiling::eLinear;
    } else if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        return vk::ImageTiling::eOptimal;
    } else {
        throw std::runtime_error("DepthStencilAttachment is not supported for requested depth format.");
    }

}

void VulkanDepthBuffer::createImage(VulkanDevice & device, uint32_t width, uint32_t height, vk::Format depthFormat, vk::ImageTiling tiling) {
    vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, depthFormat, vk::Extent3D(width, height, 1),
            1, 1, vk::SampleCountFlagBits::e1, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    buffer = device->createImageUnique(imageCreateInfo);
}

void VulkanDepthBuffer::createImageMemory(VulkanDevice & device, vk::MemoryRequirements & memoryRequirements, uint32_t typeIndex, vk::Format depthFormat) {

    depthMemory = device->allocateMemoryUnique(vk::MemoryAllocateInfo(memoryRequirements.size, typeIndex));

    device->bindImageMemory(buffer.get(), depthMemory.get(), 0);

    vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
    vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);
    depthView = device->createImageViewUnique(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), buffer.get(), vk::ImageViewType::e2D, depthFormat, componentMapping, subResourceRange));

}

// Copied from the vulkan specification, section 10.2, with slight modifications

int32_t VulkanDepthBuffer::findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties & MemoryProperties, uint32_t memoryTypeBitsRequirement, vk::MemoryPropertyFlags requiredProperties) {

    for (uint32_t memoryIndex = 0; memoryIndex < MemoryProperties.memoryTypeCount; ++memoryIndex) {

        const uint32_t memoryTypeBits = (1 << memoryIndex);
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        const vk::MemoryPropertyFlags properties = MemoryProperties.memoryTypes[memoryIndex].propertyFlags;

        const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return static_cast<int32_t> (memoryIndex);
    }

    // failed to find memory type
    return -1;
}
