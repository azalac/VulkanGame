
#ifndef _VULKAN_DESCRIPTOR
#define _VULKAN_DESCRIPTOR

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include <string>
#include <map>
#include <memory>

// Represents a vulkan descriptor
class VulkanDescriptor : public VulkanUpdatable {
public:

    vk::DescriptorSetLayoutBinding layoutBinding;

    VulkanDescriptor(int binding, int count,
            vk::DescriptorType type, vk::ShaderStageFlags stages) {
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = count;
        layoutBinding.descriptorType = type;
        layoutBinding.stageFlags = stages;
    }

    VulkanDescriptor(const VulkanDescriptor& other) = delete;

    vk::DescriptorSetLayoutBinding & get(void) {
        return layoutBinding;
    }

    bool modifiesData() override {
        return true;
    }
};

// Controls a vulkan descriptor, layout and set
class VulkanDescriptorManager {
private:

    struct Binding {
        vk::DescriptorType type;
        int binding;
        VulkanDescriptor * descriptor;
    };

    std::vector<VulkanDescriptor*> descriptors;

    std::vector<Binding> bindings;

    vk::DescriptorSetLayout layout;

    vk::DescriptorSet set;

    vk::DescriptorPool pool;

    VulkanDevice & device;

    bool editable = true;

public:

    VulkanDescriptorManager(VulkanDevice & device);

    VulkanDescriptorManager(const VulkanDescriptorManager& other) = delete;


    void addDescriptor(VulkanDescriptor * binding, bool strict = true);

    void finalize(void);

    size_t count(void) {
        return descriptors.size();
    }

    vk::DescriptorSetLayout & getLayout(void) {
        return layout;
    }

    vk::DescriptorSet & getSet(void) {
        return set;
    }

private:

    void createPool(VulkanDevice & device);

    void createSets(VulkanDevice & device);

};

// A buffer which can be accessed from within a shader
class VulkanUniformBuffer : public VulkanDescriptor {
private:

    std::unique_ptr<VulkanBuffer> buffer;

    size_t size;
    int count;

public:

    VulkanUniformBuffer(VulkanDevice * device, int binding, size_t size, int count, vk::ShaderStageFlags stages) :
    VulkanDescriptor(binding, count, vk::DescriptorType::eUniformBuffer, stages),
    buffer(new VulkanBuffer(device, size * count, vk::BufferUsageFlagBits::eUniformBuffer)),
    size(size), count(count) {

    }

    size_t objSize() {
        return size;
    }

    int objCount() {
        return count;
    }

    void * data(void) {
        return buffer->getData();
    }

    virtual void update(void * data) override {

        if (count < 1) {
            return;
        }

        buffer->update(nullptr);

        vk::DescriptorBufferInfo bufferSize(buffer->getBuffer(), 0, size);

        std::vector<vk::WriteDescriptorSet> descWrites;

        for (int i = 0; i < count; i++) {
            vk::WriteDescriptorSet descWrite;

            descWrite.dstSet = *reinterpret_cast<vk::DescriptorSet*> (data);
            descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
            descWrite.descriptorCount = 1;

            descWrite.dstBinding = layoutBinding.binding;
            descWrite.dstArrayElement = i;

            descWrite.pBufferInfo = &bufferSize;

            descWrites.push_back(descWrite);
        }

        buffer->getDevice()->updateDescriptorSets(descWrites.size(), descWrites.data(), 0, nullptr);

    }

    virtual void markNeedsUpdate(void) override {
        buffer->markNeedsUpdate();
    }

};

// A view into a uniform buffer. Wraps a void buffer into a meaningful buffer via
// a template interface
template<typename T>
class VulkanBufferView {
private:
    VulkanUniformBuffer & buffer;

public:

    VulkanBufferView(VulkanUniformBuffer & buffer) :
    buffer(buffer) {

    }

    const T & at(int i) const {
        if (i < 0 || i > buffer.objCount()) {
            throw std::runtime_error("Invalid index to buffer view (outside of range)");
        }

        return ((T*) buffer.data())[i];
    }

    T & at(int i) {
        if (i < 0 || i > buffer.objCount()) {
            throw std::runtime_error("Invalid index to buffer view (outside of range)");
        }

        return ((T*) buffer.data())[i];
    }

    const T & operator[](int i) const {
        return at(i);
    }

    T & operator[](int i) {
        return at(i);
    }

    const T * operator->(void) const {
        return &at(0);
    }

    T * operator->(void) {
        return &at(0);
    }

};

#endif /* _VULKAN_DESCRIPTOR */