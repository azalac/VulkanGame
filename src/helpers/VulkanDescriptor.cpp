/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "VulkanDescriptor.hpp"
#include "VulkanSingleCommand.hpp"

#include <algorithm>

VulkanDescriptorManager::VulkanDescriptorManager(VulkanDevice & device) :
device(device) {

}

void VulkanDescriptorManager::addDescriptor(VulkanDescriptor * desc, bool strict) {
    if (!editable) {
        throw std::runtime_error("Tried to edit a finalized descriptor manager");
    }

    int bindingSpot = desc->get().binding;
    vk::DescriptorType bindingType = desc->get().descriptorType;

    if (!(std::find(descriptors.begin(), descriptors.end(), desc) == descriptors.end())) {
        return;
    }

    descriptors.push_back(desc);

    // find the map for the descriptor
    for (auto & entry : bindings) {
        if (entry.type == bindingType && entry.binding == bindingSpot) {
            // throw exception if binding spot taken and strict mode on
            if (strict) {
                throw std::runtime_error("Tried to overwrite descriptor at binding " + bindingSpot);
            } else {
                // remove the old descriptor
                descriptors.erase(std::find(descriptors.begin(), descriptors.end(), entry.descriptor));

                // add the new descriptor
                entry.descriptor = desc;
                return;
            }

        }
    }

    bindings.push_back({bindingType, bindingSpot, desc});
}

void VulkanDescriptorManager::finalize() {
    if (!editable) {
        return;
    }

    editable = false;

    createPool(device);
    createSets(device);
}

void VulkanDescriptorManager::createPool(VulkanDevice & device) {
    vk::DescriptorPoolCreateInfo poolInfo;

    std::vector<vk::DescriptorPoolSize> sizes;
    for (auto & desc : descriptors) {

        bool found = false;

        for (auto & size : sizes) {
            if (size.type == desc->get().descriptorType) {
                size.descriptorCount++;
                found = true;
                break;
            }
        }

        if (!found) {
            sizes.push_back(vk::DescriptorPoolSize(desc->get().descriptorType, 1));
        }
    }

    poolInfo.pPoolSizes = sizes.data();
    poolInfo.maxSets = sizes.size();
    poolInfo.poolSizeCount = sizes.size();

    pool = device->createDescriptorPool(poolInfo);
}

void VulkanDescriptorManager::createSets(VulkanDevice & device) {

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    
    for(auto & desc : descriptors) {
        bindings.push_back(desc->get());
    }
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    layout = (device->createDescriptorSetLayout(layoutInfo));

    vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &layout);
    
    set = device->allocateDescriptorSets(allocInfo)[0];
}
