/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanSingleCommand.hpp
 * Author: austin-z
 *
 * Created on February 9, 2019, 9:31 PM
 */

#ifndef VULKANSINGLECOMMAND_HPP
#define VULKANSINGLECOMMAND_HPP

#include "VulkanCommandBuffer.hpp"

// A single-run command buffer
class VulkanSingleCommand {
private:
    vk::CommandBuffer buffer, *pbuffer;
    
    VulkanQueue & queue;

    VulkanDevice & device;

public:

    VulkanSingleCommand(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue);

    VulkanSingleCommand(VulkanDevice & device, VulkanCommandBuffer & buffer, VulkanQueue & queue);
    
    vk::CommandBuffer * operator->(void);

    void end(void);

    /**
     * Copies the contents of one buffer to another
     * @param device The owning device
     * @param pool A command buffer pool
     * @param queue A valid queue
     * @param src The source buffer
     * @param dst The destination buffer
     * @param n The number of bytes to copy
     * @param srcoffset The source offset
     * @param dstoffset The destination offset
     */
    static void copyBuffer(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Buffer src, vk::Buffer dst,
            vk::DeviceSize n,
            vk::DeviceSize srcoffset = 0,
            vk::DeviceSize dstoffset = 0);

    /**
     * Changes an image layout
     * @param device The owning device
     * @param pool A command buffer pool
     * @param queue A valid queue
     * @param image The image to change
     * @param format The image's byte format
     * @param from The image's current layout
     * @param to The image's required layout
     */
    static void changeImageLayout(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Image image,
            vk::Format format,
            vk::ImageLayout from, vk::ImageLayout to);

    /**
     * Copies a buffer to an image
     * @param device The owning device
     * @param pool A command buffer pool
     * @param queue A valid queue
     * @param buffer The buffer to copy from
     * @param image The image to copy to
     * @param width The image's width
     * @param height The image's height
     */
    static void copyBufferToImage(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Buffer buffer,
            vk::Image image,
            int width, int height);
};


#endif /* VULKANSINGLECOMMAND_HPP */

