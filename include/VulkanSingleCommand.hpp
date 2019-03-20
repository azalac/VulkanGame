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

    static void copyBuffer(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Buffer src, vk::Buffer dst,
            vk::DeviceSize n,
            vk::DeviceSize srcoffset = 0,
            vk::DeviceSize dstoffset = 0);

    static void changeImageLayout(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Image image,
            vk::Format format,
            vk::ImageLayout from, vk::ImageLayout to);

    static void copyBufferToImage(VulkanDevice & device, VulkanCommandBufferPool & pool, VulkanQueue & queue,
            vk::Buffer buffer,
            vk::Image image,
            int width, int height);
};


#endif /* VULKANSINGLECOMMAND_HPP */

