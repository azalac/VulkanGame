/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanRenderer.hpp
 * Author: austin-z
 *
 * Created on February 9, 2019, 9:16 PM
 */

#ifndef VULKANRENDERER_HPP
#define VULKANRENDERER_HPP

#include "VulkanRenderPipeline.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanImage.hpp"
#include "VulkanSwap.hpp"

#include <mutex>

enum class FrameState {
    AQUIRING,
    RECORDING,
    DRAWING,
    IDLE
};

class VulkanScreenBufferController {
private:
    vk::ClearValue clearValue;
    vk::Rect2D screen;

    class FrameInfo {
    public:
        vk::Semaphore renderFinished, imageAvailable;
        vk::Fence fence;
        FrameState state;
        size_t index;
        std::mutex mutex;

        FrameInfo(VulkanDevice & device, size_t frame) {
            renderFinished = device->createSemaphore(vk::SemaphoreCreateInfo());
            imageAvailable = device->createSemaphore(vk::SemaphoreCreateInfo());
            fence = device->createFence(vk::FenceCreateInfo());
            state = FrameState::IDLE;
            index = frame;
        }
    };

    std::map<int, std::unique_ptr<FrameInfo>> frames;

    VulkanDevice & device;
    VulkanSwapchain & swapchain;

    size_t currentImage = 0, frameIndex = 0, maxFrames;

public:

    VulkanScreenBufferController(VulkanDevice & device, VulkanSwapchain & swapchain,
            vk::ClearColorValue clearColor = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f},
    size_t nframes = 0) :
    device(device), swapchain(swapchain) {

        screen.offset = vk::Offset2D(0, 0);
        screen.extent = swapchain.getExtent();


        clearValue.color = clearColor;

        vk::SemaphoreCreateInfo semaInfo;

        maxFrames = nframes < 1 ? swapchain.frameCount() : nframes;

        for (size_t i = 0; i < maxFrames; i++) {
            frames[i] = std::move(std::unique_ptr<FrameInfo>(new FrameInfo(device, i)));
        }

    }

    uint32_t currentIndex() {
        return currentImage;
    }

    size_t getFrameIndex() {
        return frameIndex;
    }

    bool acquireImage(vk::Fence fence) {

        try {
            currentImage = device->acquireNextImageKHR(swapchain.getSwapchain().get(),
                    std::numeric_limits<uint64_t>::max(), frames[frameIndex]->imageAvailable, fence).value;
        } catch (std::exception) {
            return false;
        }

        return true;
    }

    bool submit(VulkanQueue & queue, vk::CommandBuffer buffer, vk::Fence fence) {
        vk::SubmitInfo submitInfo;

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &frames[frameIndex]->imageAvailable;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buffer;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &frames[frameIndex]->renderFinished;

        return queue.graphicsSubmit(submitInfo, fence) == vk::Result::eSuccess;
    }

    bool present(VulkanQueue & queue) {
        vk::PresentInfoKHR presentInfo;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &frames[frameIndex]->renderFinished;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.getSwapchain().get();

        uint32_t indices[] = {
            (uint32_t)currentImage
        };
        
        presentInfo.pImageIndices = indices;

        presentInfo.pResults = nullptr;

        try {
            return queue.presentSubmit(presentInfo) == vk::Result::eSuccess;
        } catch (std::exception) {
            return false;
        }
    }

    bool queueDraw(VulkanQueue & queue, vk::CommandBuffer buffer, bool blocking = true) {

        device->resetFences({frames[frameIndex]->fence});

        if (!submit(queue, buffer, frames[frameIndex]->fence)) {
            return false;
        }

        if (!present(queue)) {
            return false;
        }

        if (blocking) {
            device->waitForFences({frames[frameIndex]->fence}, true, std::numeric_limits<uint64_t>::max());
        }

        frameIndex = (frameIndex + 1) % maxFrames;

        return true;
    }

};

class MaterialRenderer {
private:
    VulkanQueue * queue;

    VulkanSwapchain * swapchain;
    VulkanRenderPass * renderPass;

    VulkanCommandBufferGroup * buffers;

    Material * material;

    VulkanIndexBuffer * indexBuffer;
    VulkanViewport * viewport;

    std::vector<vk::Buffer> vBuffers;

    std::set<int> recorded;

public:

    MaterialRenderer(VulkanSwapchain * swapchain, VulkanRenderPass * renderPass,
            VulkanQueue * queue, VulkanCommandBufferPool * pool, VulkanViewport * viewport,
            Material * material, VulkanIndexBuffer * indexBuffer = nullptr) {
        this->swapchain = swapchain;
        this->renderPass = renderPass;
        this->queue = queue;
        this->material = material;
        this->indexBuffer = indexBuffer;
        this->viewport = viewport;

        this->vBuffers = material->getVertexBuffers();
        this->buffers = pool->allocateGroup(2, vk::CommandBufferLevel::eSecondary);
    }

    ~MaterialRenderer() {
        delete buffers;
    }

    std::shared_ptr<char> getPushConstant(int id) {
        return material->pushconst(id);
    }

    void setIndexBuffer(VulkanIndexBuffer * indexBuffer) {
        this->indexBuffer = indexBuffer;
    }

    void checkForUpdates(size_t frame) {
        material->checkForUpdates(true);

        return;
        // if the frame hasn't been recorded yet, update it
        if (recorded.find(frame) == recorded.end()) {
            material->checkForUpdates(true);
            recorded.insert(frame);
        } else {
            // if no updates, don't record the buffer again
            if (!material->checkForUpdates()) {
                return;
            }
        }

    }

    void recordFrame(size_t frame) {
        if (this->indexBuffer == nullptr) {
            throw std::runtime_error("Index Buffer cannot be null");
        }

        vk::CommandBufferInheritanceInfo inheritance(renderPass->getRenderPass(), 0, swapchain->getFrame(frame));

        vk::CommandBuffer buffer = buffers->get(frame);

        buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance));

        buffer.setScissor(0,{viewport->getScissor()});
        buffer.setViewport(0,{viewport->getView()});

        if (material->hasDescriptors()) {
            buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    material->getPipeline()->getLayout(), 0, 1, &material->getDescriptorSet(), 0, nullptr);
        }

        if (vBuffers.size() > 0) {
            std::vector<vk::DeviceSize> offsets(vBuffers.size());
            buffer.bindVertexBuffers(0, vBuffers.size(), vBuffers.data(), offsets.data());
        }

        for (auto & pc : material->getPushConstants()) {
            buffer.pushConstants(material->getPipeline()->getLayout(),
                    pc.second.stages, pc.second.offset, pc.second.size, pc.second.ptr.get());
        }

        indexBuffer->update((void*) &material->getDescriptorSet());

        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline()->get());
        buffer.bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint16);

        buffer.drawIndexed(indexBuffer->getObjectCount(), 1, 0, 0, 0);

        buffer.end();
    }

    vk::CommandBuffer getBuffer(size_t frame) {
        return buffers->get(frame);
    }

};

class MaterialRendererBuilder {
public:

    virtual MaterialRenderer * createRenderer(Material * material, VulkanIndexBuffer * indexbuffer = nullptr) = 0;

};

#endif /* VULKANRENDERER_HPP */

