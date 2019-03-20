
#ifndef _VULKANINST_H
#define _VULKANINST_H

#define GLFW_INCLUDE_VULKAN
#define VK_PROTOTYPES
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <iostream>
#include <algorithm>
#include <set>
#include <string>

#include "Window.hpp"

const std::vector<std::string> validationLayers = {
    //"VK_LAYER_LUNARG_standard_validation"
	//,"VK_LAYER_RENDERDOC_Capture"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class VulkanValidation {
private:

    std::vector<std::string> layers;

    bool useAllValidationLayers = false;

public:

    VulkanValidation(std::vector<std::string> requestedLayers = validationLayers) {
        layers = useAllValidationLayers ? getAllValidationLayers() : requestedLayers;

        checkValidationLayerSupport(layers);
    }

    const std::vector<const char*> get(void) const {
        std::vector<const char *> layers_cstr(layers.size());

        for (size_t i = 0; i < layers.size(); i++) {
            layers_cstr[i] = layers[i].c_str();
        }

        return layers_cstr;
    }

private:

    void checkValidationLayerSupport(std::vector<std::string> layers) {

        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

        for (auto & layerName : layers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (layerName.compare(layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                std::cerr << "Warning: Validation Layer " << layerName << " not found!" << std::endl;
            }
        }
    }

    std::vector<std::string> getAllValidationLayers(void) {

        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        std::vector<std::string> layers;

        for (auto & layer : availableLayers) {
            layers.push_back(std::string(layer.layerName));
        }

        return layers;
    }

};

class VulkanInstance {
private:

    const char * appName;
    vk::UniqueInstance instance;

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    vk::DebugUtilsMessengerCreateInfoEXT dumCreateInfo;

    bool requestValidationLayers = true;

    VulkanValidation enabledLayers;

public:

    VulkanInstance(const char * appName) {
        this->appName = appName;

        createInstance();

        printLayers();
    }

    ~VulkanInstance() {
        instance->destroy();
    }

    void printLayers(void) {
        auto layers = vk::enumerateInstanceLayerProperties();
        for (auto & layer : layers) {
            std::cout << "LAYER (V" << layer.specVersion << ") " << layer.layerName << ": " << layer.description << std::endl;
        }
    }

    vk::UniqueInstance& getInstance(void) {
        return instance;
    }

    void initializeDebugMessenger(void) {
        ((PFN_vkCreateDebugUtilsMessengerEXT) instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"))
                ((VkInstance) (instance.get()), reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*> (&dumCreateInfo), nullptr, &debugMessenger);
    }

    void setRequestValidationLayers(bool reqVLayers) {
        this->requestValidationLayers = reqVLayers;
    }

    const std::vector<const char*> getValidationLayers(void) const {
        return enabledLayers.get();
    }

private:

    void createInstance(void) {

        std::vector<const char*> extensions = getRequiredExtensions();

        if (requestValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        checkExtensionSupport(extensions);


        vk::ApplicationInfo appInfo(appName, 1, "No Engine", 1, VK_API_VERSION_1_1);
        vk::InstanceCreateInfo createInfo(vk::InstanceCreateFlags(), &appInfo, 0,
                nullptr, static_cast<uint32_t> (extensions.size()), extensions.data());

        std::vector<const char*> layers = enabledLayers.get();

        createInfo.enabledLayerCount = static_cast<uint32_t> (layers.size());
        createInfo.ppEnabledLayerNames = layers.data();

        if (requestValidationLayers && layers.size() > 0) {

            dumCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            dumCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
            dumCreateInfo.pfnUserCallback = debugPrint;

            createInfo.pNext = &dumCreateInfo;
        }

        instance = vk::createInstanceUnique(createInfo);

        if (!instance) {
            throw std::runtime_error("Could not create Vulkan instance");
        }

		if (requestValidationLayers && layers.size() > 0) {
			initializeDebugMessenger();
		}
    }

    void checkExtensionSupport(std::vector<const char*> & extensions) {

        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

        for (const char* ext : extensions) {
            bool extensionFound = false;

            for (const auto& extProperties : availableExtensions) {
                if (strcmp(ext, extProperties.extensionName) == 0) {
                    extensionFound = true;
                    break;
                }
            }

            if (!extensionFound) {
                throw std::runtime_error("Vulkan extension " + std::string(ext) + " not found");
            }
        }
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        return extensions;
    }

    static vk::Bool32 cppDebugPrint(vk::DebugUtilsMessageSeverityFlagsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT types,
            vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData) {



        return VK_FALSE;
    }

    static VkBool32 VKAPI_PTR debugPrint(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        fprintf(stderr, "[%s]: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);

        return VK_FALSE;
    }
};

#endif /* _VULKANINST_H */

