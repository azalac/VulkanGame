
#ifndef _VULKAN_SWAP
#define _VULKAN_SWAP

#include "VulkanDevice.hpp"
#include "VulkanRenderPass.hpp"

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

class VulkanSwapchain {
private:

    vk::UniqueSwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> imageViews;

	std::vector<vk::Framebuffer> frameBuffers;

    vk::Extent2D extent;

public:

    VulkanSwapchain(VulkanDevice & device, VulkanViewport & viewport, VulkanRenderPass & renderPass) {

        extent = getSurfaceExtent(viewport, device.getSurfaceCapabilities());

        createSwapchain(device.getSurface(), extent,
                device.getSurfaceCapabilities(), device.getSurfaceFormat(), device.getGraphicsQueueIndex(),
                device.getPresentQueueIndex(), device);

        createImageViews(device);

		createFrameBuffers(device, renderPass);
    }

    void destroy(VulkanDevice & device) {

        device->destroySwapchainKHR(swapChain.get());
		swapChain.release();

		for (auto & frame : frameBuffers) {
			device->destroyFramebuffer(frame);
			frame = nullptr;
		}

    }

    void recreateSwapchain(VulkanDevice & device, VulkanViewport & viewport, VulkanRenderPass & renderPass) {

        swapChainImages.clear();
        imageViews.clear();

        extent = getSurfaceExtent(viewport, device.getSurfaceCapabilities());

        createSwapchain(device.getSurface(), extent,
                device.getSurfaceCapabilities(), device.getSurfaceFormat(), device.getGraphicsQueueIndex(),
                device.getPresentQueueIndex(), device);

        createImageViews(device);

		createFrameBuffers(device, renderPass);
    }

    size_t frameCount() {
        return swapChainImages.size();
    }

    const vk::Extent2D getExtent(void) const {
        return extent;
    }

    const std::vector<vk::ImageView> & getImageViews(void) const {
        return imageViews;
    }

    const vk::ImageView & getImageView(int frame) const {
        return imageViews[frame];
    }

    vk::UniqueSwapchainKHR & getSwapchain(void) {
        return swapChain;
    }

	vk::Framebuffer & getFrame(int i) {
		return frameBuffers[i];
	}

private:

    vk::Extent2D getSurfaceExtent(VulkanViewport & viewport, vk::SurfaceCapabilitiesKHR capabilities) {
        if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
            // If the surface size is undefined, the size is set to the size of the images requested.
            return vk::Extent2D(clamp(viewport.getWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                    clamp(viewport.getHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
        } else {
            // If the surface size is defined, the swap chain size must match
            return capabilities.currentExtent;
        }
    }

    vk::PresentModeKHR findBestPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes) {
        vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    void createSwapchain(vk::SurfaceKHR surface, vk::Extent2D extent, vk::SurfaceCapabilitiesKHR capabilities,
            vk::Format format, uint32_t graphicsQueueIndex, uint32_t presentQueueIndex, VulkanDevice & device) {

        vk::PresentModeKHR swapchainPresentMode = findBestPresentMode(device.getSurfacePresentModes());

        vk::SurfaceTransformFlagBitsKHR preTransform = (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
                vk::SurfaceTransformFlagBitsKHR::eIdentity : capabilities.currentTransform;

        vk::CompositeAlphaFlagBitsKHR compositeAlpha =
                (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
                (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
                (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), surface, capabilities.minImageCount, format,
                vk::ColorSpaceKHR::eSrgbNonlinear, extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr,
                preTransform, compositeAlpha, swapchainPresentMode, true);

        uint32_t queueFamilyIndices[2] = {graphicsQueueIndex, presentQueueIndex};
        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            // If the graphics and present queues are from different queue families, we either have to explicitly transfer ownership of images between
            // the queues, or we have to create the swapchain with imageSharingMode as VK_SHARING_MODE_CONCURRENT
            swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

		if (swapChain) {
			device->destroySwapchainKHR(swapChain.get());
		}

        swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);

        swapChainImages = device->getSwapchainImagesKHR(swapChain.get());
    }

    void createImageViews(VulkanDevice & device) {
        imageViews.reserve(swapChainImages.size());

        for (auto image : swapChainImages) {
            vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
            vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, device.getSurfaceFormat(), componentMapping, subResourceRange);

            vk::ImageView view = device->createImageView(imageViewCreateInfo);
            imageViews.push_back(view);


            if (!view) {
                throw std::runtime_error("Couldn't create ImageView");
            }

        }
    }

	void createFrameBuffers(VulkanDevice & device, VulkanRenderPass & renderPass) {
		frameBuffers.resize(imageViews.size());
		vk::FramebufferCreateInfo createInfo(vk::FramebufferCreateFlags(), renderPass.getRenderPass(), 1, nullptr, extent.width, extent.height, 1);

		for (size_t i = 0; i < imageViews.size(); i++) {
			if (frameBuffers[i]) {
				device->destroyFramebuffer(frameBuffers[i]);
			}

			createInfo.pAttachments = &imageViews[i];

			frameBuffers[i] = device->createFramebuffer(createInfo);
		}
	}

};

#endif