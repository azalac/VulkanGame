/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanRenderPass.hpp"

void VulkanRenderPass::addAttachment(vk::AttachmentDescription attachment, std::string name) {
    attachments.push_back(attachment);
    att_names.push_back(name);
}

vk::AttachmentReference VulkanRenderPass::getAttachmentReference(std::string name, vk::ImageLayout layout) {
    for (size_t i = 0; i < attachments.size(); i++) {
        if (att_names[i] == name) {
            return vk::AttachmentReference(i, layout);
        }
    }
    throw std::runtime_error("Attachment " + name + " is not defined");
}

void VulkanRenderPass::createDefaultAttachments(VulkanDevice & device, VulkanDepthBuffer & depthBuffer) {

    vk::AttachmentDescription surfaceAttachment;

    surfaceAttachment.format = device.getSurfaceFormat();
    surfaceAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    surfaceAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    surfaceAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    surfaceAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    surfaceAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    addAttachment(surfaceAttachment, "surface");

    vk::AttachmentDescription depthAttachment;

    depthAttachment.format = depthBuffer.getBufferFormat();
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    //addAttachment(depthAttachment, "depth");
}

void VulkanRenderPass::createRenderPass(VulkanDevice & device) {
    vk::AttachmentReference colorReference = getAttachmentReference("surface", vk::ImageLayout::eColorAttachmentOptimal);
    //vk::AttachmentReference depthReference = getAttachmentReference("depth", vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass;

    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    //subpass.pDepthStencilAttachment = &depthReference;


    vk::SubpassDependency subpassDep;

    subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDep.dstSubpass = 0;

    subpassDep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDep.srcAccessMask = vk::AccessFlags();
    subpassDep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo(vk::RenderPassCreateFlags(), attachments.size(), attachments.data(), 1, &subpass, 1, &subpassDep);

    renderPass = device->createRenderPassUnique(renderPassInfo);
}