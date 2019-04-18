/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanRenderPass.hpp
 * Author: austin-z
 *
 * Created on February 9, 2019, 9:47 PM
 */

#ifndef VULKANRENDERPASS_HPP
#define VULKANRENDERPASS_HPP

#include "VulkanDevice.hpp"
#include "VulkanDepthBuffer.hpp"

#include <vector>

// Controls a render pass and its attachments
class VulkanRenderPass {
private:

    std::vector<vk::AttachmentDescription> attachments;
    std::vector<std::string> att_names;

    vk::UniqueRenderPass renderPass;

public:

    VulkanRenderPass(VulkanDevice & device, VulkanDepthBuffer & depthBuffer) {
        createDefaultAttachments(device, depthBuffer);

        createRenderPass(device);
    }

    void addAttachment(vk::AttachmentDescription attachment, std::string name);

    vk::AttachmentReference getAttachmentReference(std::string name, vk::ImageLayout layout);

    vk::RenderPass & getRenderPass(void) {
        return renderPass.get();
    }

private:

    void createDefaultAttachments(VulkanDevice & device, VulkanDepthBuffer & depthBuffer);

    void createRenderPass(VulkanDevice & device);
};

#endif /* VULKANRENDERPASS_HPP */
