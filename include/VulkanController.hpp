/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanController.hpp
 * Author: austin-z
 *
 * Created on February 10, 2019, 12:28 AM
 */

#ifndef VULKANCONTROLLER_HPP
#define VULKANCONTROLLER_HPP

#include "VulkanInst.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwap.hpp"
#include "VulkanDepthBuffer.hpp"
#include "VulkanRenderPipeline.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanSingleCommand.hpp"

#include "VulkanImage.hpp"

// Controls the initialization and deinitialization of various vulkan objects
class VulkanController : public MaterialRendererBuilder {
private:

    Window * window;
    VulkanInstance * instance;
    VulkanDevice * device;
    VulkanSwapchain * swapchain;
    VulkanDepthBuffer * depthBuffer;
    VulkanRenderPass * renderPass;
    VulkanCommandBufferPool * cmdpool;
    VulkanQueue * queue;
    VulkanViewport * viewport;
    VulkanScreenBufferController * screenController;

    ImageManager * images;

    vk::Fence acquire_fence;
    vk::CommandBuffer pBuffer;

public:

    VulkanController(Window & wnd) {
        window = &wnd;
        instance = new VulkanInstance(wnd.getTitle());

        viewport = new VulkanViewport(wnd);

        device = new VulkanDevice(*instance, wnd);

        cmdpool = new VulkanCommandBufferPool(*device);

        queue = new VulkanQueue(*device);

        depthBuffer = new VulkanDepthBuffer(*device, *viewport);

        renderPass = new VulkanRenderPass(*device, *depthBuffer);

        swapchain = new VulkanSwapchain(*device, *viewport, *renderPass);

        screenController = new VulkanScreenBufferController(*device, *swapchain);

        images = new ImageManager(device, cmdpool, queue);

        pBuffer = cmdpool->create(vk::CommandBufferLevel::ePrimary);

        acquire_fence = (*device)->createFence(vk::FenceCreateInfo());
    }

    VulkanController(const VulkanController& other) = delete;

    virtual ~VulkanController() {
        (*device)->destroyFence(acquire_fence);
        (*device)->freeCommandBuffers(*cmdpool,{pBuffer});
        delete queue;
        delete cmdpool;
        delete depthBuffer;
        delete swapchain;
        delete renderPass;
        delete device;
        delete instance;
    }

    void recreateSwapchain(void) {
        // recreate the data without touching the pointers

        (*device)->waitIdle();

        viewport->reload(*window);

        swapchain->destroy(*device);

        device->recreateSurface(*instance, *window);

        swapchain->recreateSwapchain(*device, *viewport, *renderPass);

        queue->revalidateQueues(*device);
    }

    VulkanDevice * getDevice(void) {
        return device;
    }

    VulkanViewport * getViewport(void) {
        return viewport;
    }

    ImageManager * getImageManager(void) {
        return images;
    }

    VulkanSwapchain * getSwapchain(void) {
        return swapchain;
    }

    VulkanRenderPass * getRenderPass(void) {
        return renderPass;
    }

    VulkanQueue * getQueue(void) {
        return queue;
    }

    VulkanCommandBufferPool * getBufferPool(void) {
        return cmdpool;
    }

    size_t getFrameIndex(void) {
        return screenController->currentIndex();
    }

    Material * createMaterial(MaterialPrototype & prototype, UBOMap * globals = nullptr) {
        return new Material(*device, prototype, *viewport, *renderPass, *queue, globals);
    }

    virtual MaterialRenderer * createRenderer(Material * material, VulkanIndexBuffer * indexbuffer = nullptr) override {
        return new MaterialRenderer(swapchain, renderPass, queue, cmdpool, viewport, material, indexbuffer);
    }

    /**
     * Acquires the next image
     */
    void startRender(void) {

        (*device)->resetFences({acquire_fence});

        if (!screenController->acquireImage(acquire_fence)) {
            printf("Warning: no image acquired");
            return;
        }

        (*device)->waitForFences({acquire_fence}, true, std::numeric_limits<uint64_t>::max());

    }

    /**
     * Submits a list of secondary buffers to be drawn
     * @param secondaries The list of secondary buffers
     */
    void submitSecondaries(std::vector<vk::CommandBuffer> & secondaries) {

        pBuffer.reset(vk::CommandBufferResetFlags());

        pBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        vk::ClearValue cclear;

        cclear.color = std::array<float, 4>({0.0f, 0.0f, 0.0f, 0.0f});

        vk::RenderPassBeginInfo info(renderPass->getRenderPass(), swapchain->getFrame(getFrameIndex()), viewport->getScissor(), 1, &cclear);

        pBuffer.beginRenderPass(info, vk::SubpassContents::eSecondaryCommandBuffers);

        pBuffer.executeCommands(secondaries.size(), secondaries.data());

        pBuffer.endRenderPass();

        pBuffer.end();

        screenController->queueDraw(*queue, pBuffer);
    }

};


#endif /* VULKANCONTROLLER_HPP */

