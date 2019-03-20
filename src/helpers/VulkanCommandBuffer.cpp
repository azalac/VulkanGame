/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanCommandBuffer.hpp"

VulkanCommandBuffer::VulkanCommandBuffer(vk::CommandBuffer buffer, bool primary) {
    this->buffer = buffer;
    this->primary = primary;
}


const bool VulkanCommandBuffer::is_primary(void) const {
    return primary;
}

const bool VulkanCommandBuffer::allocated(void) const {
    return allocation != -1;
}

const bool VulkanCommandBuffer::owned_by(int id) const {
    return id == allocation;
}

void VulkanCommandBuffer::allocate(int group) {
    if (allocated()) {
        throw std::runtime_error("Cannot reallocate allocated buffer");
    }

    allocation = group;
}

void VulkanCommandBuffer::deallocate(void) {
    allocation = -1;
}

vk::CommandBuffer* VulkanCommandBuffer::operator->(void) {
    return &buffer;
}

vk::CommandBuffer& VulkanCommandBuffer::get(void) {
    return buffer;
}

VulkanCommandBufferGroup::VulkanCommandBufferGroup(int id, std::vector<VulkanCommandBuffer*> buffers, bool primary) {
    this->id = id;
    this->buffers = buffers;
    this->primary = primary;
}

VulkanCommandBufferGroup::~VulkanCommandBufferGroup() {
    for (auto buffer : buffers) {
        buffer->deallocate();
    }
}

vk::CommandBuffer VulkanCommandBufferGroup::operator[](int i) {
    return buffers[i]->get();
}

vk::CommandBuffer VulkanCommandBufferGroup::get(int i) {
    return buffers[i]->get();
}

size_t VulkanCommandBufferGroup::count(void) {
    return buffers.size();
}

std::vector<vk::CommandBuffer> VulkanCommandBufferGroup::getBuffers(void) {
    std::vector<vk::CommandBuffer> out;

    for (auto & vcmd : buffers) {
        out.push_back(vcmd->get());
    }

    return out;
}

VulkanCommandBufferPool::VulkanCommandBufferPool(VulkanDevice & device, size_t initial_buffers) :
device(device) {

    vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, device.getGraphicsQueueIndex());

    pool = device->createCommandPool(createInfo);

    if (!pool) {
        throw std::runtime_error("Could not create command pool");
    }

    allocatePrimaryBuffers(initial_buffers);
    allocateSecondaryBuffers(initial_buffers);
}

VulkanCommandBufferPool::~VulkanCommandBufferPool() {

    for (auto & buffer : pbuffers) {
		device->freeCommandBuffers(pool, { buffer->get() });
    }

    for (auto & buffer : sbuffers) {
		device->freeCommandBuffers(pool, { buffer->get() });
    }

}

void VulkanCommandBufferPool::allocatePrimaryBuffers(size_t count) {

    for (auto & buffer : pbuffers) {
		device->freeCommandBuffers(pool, { buffer->get() });
        delete buffer;
    }

    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = count;

    std::vector<vk::CommandBuffer> buffers = device->allocateCommandBuffers(allocInfo);

    pbuffers.resize(buffers.size());

    for (size_t i = 0; i < buffers.size(); i++) {
        pbuffers[i] = new VulkanCommandBuffer(buffers[i]);
    }
}

void VulkanCommandBufferPool::addMorePrimaryBuffers(size_t extra) {

    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = extra;

    std::vector<vk::CommandBuffer> buffers = device->allocateCommandBuffers(allocInfo);

    for (auto & buffer : buffers) {
        pbuffers.push_back(new VulkanCommandBuffer(buffer));
    }
}

void VulkanCommandBufferPool::allocateSecondaryBuffers(size_t count) {

    for (auto & buffer : pbuffers) {
		device->freeCommandBuffers(pool, { buffer->get() });
        delete buffer;
    }

    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::eSecondary;
    allocInfo.commandBufferCount = count;

    std::vector<vk::CommandBuffer> buffers = device->allocateCommandBuffers(allocInfo);

    sbuffers.resize(buffers.size());

    for (size_t i = 0; i < buffers.size(); i++) {
        sbuffers[i] = new VulkanCommandBuffer(buffers[i], false);
    }
}

void VulkanCommandBufferPool::addMoreSecondaryBuffers(size_t extra) {

    vk::CommandBufferAllocateInfo allocInfo;

    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::eSecondary;
    allocInfo.commandBufferCount = extra;

    std::vector<vk::CommandBuffer> buffers = device->allocateCommandBuffers(allocInfo);

    for (auto & buffer : buffers) {
        sbuffers.push_back(new VulkanCommandBuffer(buffer, false));
    }
}

VulkanCommandBufferGroup * VulkanCommandBufferPool::allocateGroup(size_t count, vk::CommandBufferLevel level) {
    std::vector<VulkanCommandBuffer*> buffers;
    
    bool primary = level == vk::CommandBufferLevel::ePrimary;

    // try and find <count> unallocated buffers
    for (auto & buffer : (primary ? pbuffers : sbuffers)) {
        if (!buffer->allocated()) {
            buffers.push_back(buffer);
        }
        if (buffers.size() == count) {
            break;
        }
    }

    if (buffers.size() != count) {
        if (primary) {
            addMorePrimaryBuffers(count);
        } else {
            addMoreSecondaryBuffers(count);
        }

        return allocateGroup(count, level);
    }

    int group = nextgroup++;

    for (auto & buffer : buffers) {
        buffer->allocate(group);
    }

    return new VulkanCommandBufferGroup(group, buffers, primary);
}

VulkanCommandBufferPool::operator vk::CommandPool&(void) {
    return pool;
}
