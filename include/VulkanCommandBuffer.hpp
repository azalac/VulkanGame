
#ifndef _VULKAN_COMMAND_BUFFER
#define _VULKAN_COMMAND_BUFFER

#include "VulkanDevice.hpp"

#include <vector>

class VulkanCommandBuffer {
private:

    vk::CommandBuffer buffer;
    int allocation = -1;
    bool primary;

public:

    VulkanCommandBuffer(vk::CommandBuffer buffer, bool primary = true);

    VulkanCommandBuffer(const VulkanCommandBuffer& other) = delete;
	
    const bool is_primary(void) const;

    const bool allocated(void) const;

    const bool owned_by(int id) const;

    void allocate(int group);

    void deallocate(void);

    vk::CommandBuffer* operator->(void);

    vk::CommandBuffer& get(void);

};

class VulkanCommandBufferGroup {
private:

    int id;
    
    std::vector<VulkanCommandBuffer*> buffers;
    bool primary = true;

public:

    VulkanCommandBufferGroup(int id, std::vector<VulkanCommandBuffer*> buffers, bool primary);

    ~VulkanCommandBufferGroup();

    VulkanCommandBufferGroup(const VulkanCommandBufferGroup& other) = delete;

    vk::CommandBuffer operator[](int i);
    
    vk::CommandBuffer get(int i);

    size_t count(void);

    std::vector<vk::CommandBuffer> getBuffers(void);

    std::vector<VulkanCommandBuffer*> & getBuffersInternal(void) {
        return buffers;
    }
};

class VulkanCommandBufferPool {
private:

    vk::CommandPool pool;

    std::vector<VulkanCommandBuffer*> pbuffers, sbuffers;

    int nextgroup = 0;

    VulkanDevice & device;

public:

    VulkanCommandBufferPool(VulkanDevice & device, size_t initial_buffers = 6);

    virtual ~VulkanCommandBufferPool();

	vk::CommandBuffer create(vk::CommandBufferLevel level) {
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = pool;
		allocInfo.level = level;
		return device->allocateCommandBuffers(allocInfo)[0];
	}

    VulkanCommandBufferGroup * allocateGroup(size_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

    operator vk::CommandPool&(void);

    vk::CommandPool & get(void) {
        return pool;
    }
    
private:
    void allocatePrimaryBuffers(size_t count);

    void addMorePrimaryBuffers(size_t extra);

    void allocateSecondaryBuffers(size_t count);

    void addMoreSecondaryBuffers(size_t extra);

};

#endif /*_VULKAN_COMMAND_BUFFER */
