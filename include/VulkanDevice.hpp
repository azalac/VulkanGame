#pragma once

#include "VulkanInst.hpp"

#ifndef _VULKAN_DEVICE
#define _VULKAN_DEVICE

// Controls a vulkan device and configures the rendering surface

class VulkanDevice {
private:
    vk::SurfaceKHR surface;
    vk::Format surfaceFormat = vk::Format::eUndefined;
    vk::SurfaceCapabilitiesKHR surfaceCapabilities;

    std::vector<vk::PhysicalDevice> pdevices;

    vk::PhysicalDevice * physical_device;
    vk::Device logical_device;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    vk::DeviceCreateInfo dCreateInfo;

    size_t graphicsQueueFamilyIndex = 0;
    size_t presentQueueFamilyIndex = 0;

public:

    VulkanDevice(VulkanInstance & instance, Window & wnd, vk::QueueFlagBits reqProperties = vk::QueueFlagBits::eGraphics) {
        createSurface(instance, wnd);
        findPhysicalDevice(instance, reqProperties);
    }

    vk::SurfaceKHR & getSurface(void) {
        return surface;
    }

    uint32_t getGraphicsQueueIndex(void) {
        return graphicsQueueFamilyIndex;
    }

    uint32_t getPresentQueueIndex(void) {
        return presentQueueFamilyIndex;
    }

    vk::Format getSurfaceFormat(void) {
        return findSurfaceFormat(*physical_device);
    }

    vk::SurfaceCapabilitiesKHR getSurfaceCapabilities(void) {
        return physical_device->getSurfaceCapabilitiesKHR(surface);
    }

    std::vector<vk::PresentModeKHR> getSurfacePresentModes(void) {
        return physical_device->getSurfacePresentModesKHR(surface);
    }

    vk::FormatProperties getFormatProperties(const vk::Format & format) {
        return physical_device->getFormatProperties(format);
    }

    vk::PhysicalDeviceMemoryProperties getMemoryProperties(void) {
        return physical_device->getMemoryProperties();
    }

    bool supportsAnisotropy(void) {
        return physical_device->getFeatures().samplerAnisotropy;
    }

    vk::Device const* operator->(void) const {
        return &logical_device;
    }

    vk::Device * operator->(void) {
        return &logical_device;
    }

    vk::Device & operator*(void) {
        return logical_device;
    }

    void printExtensions(void) {
        auto extensions = physical_device->enumerateDeviceExtensionProperties();
        for (auto & extension : extensions) {
            std::cout << "EXTENSION (V" << extension.specVersion << ") " << extension.extensionName << std::endl;
        }
    }

    void recreateSurface(VulkanInstance & instance, Window & wnd) {
        instance.getInstance()->destroySurfaceKHR(surface);

        createSurface(instance, wnd);

        findValidDeviceIndices(*physical_device, physical_device->getQueueFamilyProperties(),
                vk::QueueFlagBits::eGraphics, &graphicsQueueFamilyIndex, &presentQueueFamilyIndex);
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physical_device->getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        buffer = logical_device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = logical_device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        bufferMemory = logical_device.allocateMemory(allocInfo);

        logical_device.bindBufferMemory(buffer, bufferMemory, 0);
    }


private:

    void createSurface(VulkanInstance & instance, Window & wnd) {
        VkSurfaceKHR surf;

        if (glfwCreateWindowSurface(static_cast<VkInstance> (instance.getInstance().get()), wnd.getWindow(), nullptr, &surf) != VK_SUCCESS) {
            throw std::runtime_error("Could not create window surface");
        }

        this->surface = vk::SurfaceKHR(vk::SurfaceKHR(surf));
    }

    void findPhysicalDevice(VulkanInstance & instance, vk::QueueFlagBits reqProperties) {

        pdevices = instance.getInstance()->enumeratePhysicalDevices();

        if (pdevices.size() == 0) {
            throw std::runtime_error("No GPUs with Vulkan support found");
        }

        bool found = false;

        std::vector<const char*> layers = instance.getValidationLayers();

        for (auto& device : pdevices) {
            try {
                tryGetLogicalDevice(device, reqProperties, layers);
                physical_device = &device;
                found = true;
                break;
            } catch (std::exception & e) {
                std::cout << "Device " << device.getProperties().deviceName << " is not a valid device (" << e.what() << ")" << std::endl;
            }
        }

        if (!found) {
            throw std::runtime_error("Could not find a valid device");
        }

    }

    void tryGetLogicalDevice(const vk::PhysicalDevice & device, const vk::QueueFlagBits reqProperties, std::vector<const char*> validation) {

        // find logical devices which support graphics and presenting
        std::vector<vk::QueueFamilyProperties> properties = device.getQueueFamilyProperties();
        if (!findValidDeviceIndices(device, properties, reqProperties, &graphicsQueueFamilyIndex, &presentQueueFamilyIndex)) {
            throw std::runtime_error("No queues have requested properties");
        }

        // create the graphics device
        float queuePriority = 0.0f;
        deviceQueueCreateInfo = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), static_cast<uint32_t> (graphicsQueueFamilyIndex), 1, &queuePriority);

        dCreateInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size());
        dCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        dCreateInfo.enabledLayerCount = static_cast<uint32_t> (validation.size());
        dCreateInfo.ppEnabledLayerNames = validation.data();

        dCreateInfo.queueCreateInfoCount = 1;
        dCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

        logical_device = device.createDevice(dCreateInfo);

    }

    bool deviceSupportsExtensions(const vk::PhysicalDevice & device) {

        std::vector<vk::ExtensionProperties> availExtensions = device.enumerateDeviceExtensionProperties();

        for (const auto &rext : deviceExtensions) {
            bool found = false;

            for (const auto &aext : availExtensions) {
                if (strcmp(rext, aext.extensionName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                return false;
            }

        }

        return true;
    }

    vk::Format findSurfaceFormat(vk::PhysicalDevice & device) {

        std::vector<vk::SurfaceFormatKHR> formats = device.getSurfaceFormatsKHR(surface);

        if (formats.empty()) {
            throw std::runtime_error("No supported surface formats");
        }

        size_t i = 0;

        while (formats[i].format == vk::Format::eUndefined && i < formats.size()) {
            i++;
        }

        return i < formats.size() ? formats[i].format : vk::Format::eB8G8R8A8Unorm;
    }

    bool findValidDeviceIndices(const vk::PhysicalDevice & device, std::vector<vk::QueueFamilyProperties> properties,
            vk::QueueFlagBits reqProperties, size_t * graphicsIndex, size_t * presentIndex) {

        // find the first device which matches the required properties
        *graphicsIndex = std::distance(properties.begin(),
                std::find_if(properties.begin(),
                properties.end(),
                [reqProperties](vk::QueueFamilyProperties const& props) {
                    return props.queueFlags & reqProperties; }));

        // no valid renderer device found
        if (*graphicsIndex == properties.size()) {
            return false;
        }

        //size_t presentQueueFamilyIndex = properties.size();

        bool presentFound = false;

        // check if the current graphics ldevice supports presenting
        if (device.getSurfaceSupportKHR(static_cast<uint32_t> (*graphicsIndex), surface)) {
            //presentQueueFamilyIndex = *graphicsIndex;
            presentFound = true;
        } else {

            // if the current ldevice doesn't support presenting, find one that does
            for (size_t i = 0; i < properties.size(); i++) {
                if ((properties[i].queueFlags & reqProperties) && device.getSurfaceSupportKHR(static_cast<uint32_t> (i), surface)) {
                    *graphicsIndex = i;
                    *presentIndex = i;
                    presentFound = true;
                    break;
                }
            }

            if (!presentFound) {
                // there's no i which supports both the properties and surface, find a present that does
                for (size_t i = 0; i < properties.size(); i++) {
                    if (device.getSurfaceSupportKHR(static_cast<uint32_t> (i), surface)) {
                        *presentIndex = i;
                        presentFound = true;
                        break;
                    }
                }
            }
        }

        return presentFound;

    }

};

// Controls the present and graphics command queues

class VulkanQueue {
private:

    int graphicsIndex = -1, presentIndex = -1;

    vk::Queue graphics = nullptr, present = nullptr;

public:

    VulkanQueue(VulkanDevice & device) {
        revalidateQueues(device);
    }

    void revalidateQueues(VulkanDevice & device) {
        graphicsIndex = device.getGraphicsQueueIndex();
        presentIndex = device.getPresentQueueIndex();

        graphics = device->getQueue(graphicsIndex, 0);
        present = device->getQueue(presentIndex, 0);
    }

    vk::Result graphicsSubmit(vk::SubmitInfo & submitInfo, vk::Fence fence) {
        return graphics.submit(1, &submitInfo, fence);
    }

    vk::Result presentSubmit(vk::PresentInfoKHR & presentInfo) {
        return present.presentKHR(presentInfo);
    }

};

// Controls the viewport information
class VulkanViewport {
private:

    vk::PipelineViewportStateCreateInfo viewportState;

    vk::Viewport view;
    vk::Rect2D scissor;

    uint32_t width = 0, height = 0;

public:

    VulkanViewport(void) {

    }

    VulkanViewport(Window & wnd) {
        reload(wnd);
    }

    void reload(Window & wnd) {

        width = wnd.getWidth();
        height = wnd.getHeight();

        view.x = 0;
        view.y = 0;
        view.width = (float) width;
        view.height = (float) height;
        view.minDepth = 0;
        view.maxDepth = 1;

        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = vk::Extent2D(width, height);

        viewportState.viewportCount = 1;
        viewportState.pViewports = &view;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

    }

    uint32_t getHeight() const {
        return height;
    }

    const vk::Viewport & getView() const {
        return view;
    }

    const vk::Rect2D & getScissor() const {
        return scissor;
    }

    uint32_t getWidth() const {
        return width;
    }

    vk::PipelineViewportStateCreateInfo getViewportState(void) {
        return viewportState;
    }
};

#endif /* _VULKAN_DEVICE */
