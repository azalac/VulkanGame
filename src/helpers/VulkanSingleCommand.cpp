/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanSingleCommand.hpp"

VulkanSingleCommand::VulkanSingleCommand(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue) :
queue(queue), device(device) {

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = pool.get();

    buffer = device->allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    buffer.begin(beginInfo);
    pbuffer = &buffer;

}

VulkanSingleCommand::VulkanSingleCommand(VulkanDevice & device, VulkanCommandBuffer & buffer, VulkanQueue & queue) :
queue(queue), device(device) {
    
    buffer->begin(vk::CommandBufferBeginInfo());
    
    pbuffer = &buffer.get();
}

vk::CommandBuffer * VulkanSingleCommand::operator->(void) {
    return pbuffer;
}

void VulkanSingleCommand::end(void) {
    pbuffer->end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = pbuffer;

    vk::Fence waitFence = device->createFence(vk::FenceCreateInfo());

    queue.graphicsSubmit(submitInfo, waitFence);

    device->waitForFences(1, &waitFence, VK_FALSE, std::numeric_limits<uint64_t>::max());
}

void VulkanSingleCommand::copyBuffer(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
        vk::Buffer src, vk::Buffer dst, vk::DeviceSize n, vk::DeviceSize srcoffset, vk::DeviceSize dstoffset) {
    VulkanSingleCommand cmd(device, pool, queue);

    vk::BufferCopy region(srcoffset, dstoffset, n);

    cmd->copyBuffer(src, dst, 1, &region);

    cmd.end();
}

void VulkanSingleCommand::changeImageLayout(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
        vk::Image image, vk::Format format, vk::ImageLayout from, vk::ImageLayout to) {
    VulkanSingleCommand cmd(device, pool, queue);

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = from;
    barrier.newLayout = to;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;


    if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlags();
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (from == vk::ImageLayout::eTransferDstOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    cmd->pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

    cmd.end();
}

void VulkanSingleCommand::copyBufferToImage(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
        vk::Buffer buffer, vk::Image image, int width, int height) {
    VulkanSingleCommand cmd(device, pool, queue);

    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D(0, 0, 0);
    region.imageExtent = vk::Extent3D(width, height, 1);

    cmd->copyBufferToImage(buffer,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &region);

    cmd.end();
}