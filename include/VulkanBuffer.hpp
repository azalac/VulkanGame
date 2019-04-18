/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanBuffer.hpp
 * Author: austin-z
 *
 * Created on February 12, 2019, 9:42 PM
 */

#ifndef VULKANBUFFER_HPP
#define VULKANBUFFER_HPP

#include "VulkanDevice.hpp"

#include <memory>

// An object which holds a vulkan buffer and its memory

class VulkanBufferHolder {
protected:
    vk::Buffer buffer;
    vk::DeviceMemory memory;

public:

    VulkanBufferHolder(vk::Buffer buffer, vk::DeviceMemory memory = nullptr) {
        this->buffer = buffer;
        this->memory = memory;
    }

    virtual vk::Buffer getBuffer(void) {
        return buffer;
    }

    virtual vk::DeviceMemory getMemory(void) {
        return memory;
    }
};

// An object which creates a vulkan buffer and its memory

class VulkanBufferCreator : public VulkanBufferHolder {
public:

    VulkanBufferCreator(VulkanDevice * device, int size, vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) :
    VulkanBufferHolder(nullptr) {
        device->createBuffer(size, usage, memoryProperties, buffer, memory);
    }

};

// Represents an updatable resource

class VulkanUpdatable {
private:

    bool objectDirty = true;

public:

    bool needsUpdate(void) {
        return objectDirty;
    }

    void markUpdated(void) {
        objectDirty = false;
    }

    virtual void markNeedsUpdate(void) {
        objectDirty = true;
    }

    virtual void update(void* data) = 0;

    virtual bool modifiesData(void) = 0;

};

// Controls a vulkan buffer

class VulkanBuffer : public VulkanUpdatable {
private:

    std::unique_ptr<VulkanBufferHolder> holder;

public:

    VulkanBuffer(VulkanDevice * device, int size, vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent) :
    holder(new VulkanBufferCreator(device, size, usage, memoryProperties)) {

        this->device = device;

        data = new char[size];
        this->size = size;

    }

    VulkanBuffer(vk::Buffer buffer, VulkanDevice * device = nullptr, vk::DeviceMemory memory = nullptr) :
    holder(new VulkanBufferHolder(buffer, memory)) {

        this->device = device;

        data = nullptr;
        this->size = 0;

    }

    virtual ~VulkanBuffer() {
        delete[] data;
    }

    virtual void update(void* data2) {
        if (data) {
            obj2buf(data, size);
        }
    }

    virtual bool modifiesData(void) {
        return true;
    }

    vk::Buffer getBuffer(void) {
        return holder->getBuffer();
    }

    void * getData(void) {
        return data;
    }

    size_t getSize(void) {
        return size;
    }

    VulkanDevice & getDevice(void) {
        return *device;
    }

protected:

    char * data;

    size_t size;

    VulkanDevice * device;

    void obj2buf(void * data, size_t count) {
        vk::DeviceMemory memory = holder->getMemory();

        if (!memory) {
            throw std::runtime_error("No memory in buffer holder object, cannot map memory");
        }

        void * ptr = (*device)->mapMemory(memory, 0, count, vk::MemoryMapFlags());
        memcpy(ptr, data, count);
        (*device)->unmapMemory(memory);
    }

    void buf2obj(void * data, size_t count) {
        vk::DeviceMemory memory = holder->getMemory();

        if (!memory) {
            throw std::runtime_error("No memory in buffer holder object, cannot map memory");
        }

        void * ptr = (*device)->mapMemory(memory, 0, count, vk::MemoryMapFlags());
        memcpy(data, ptr, count);
        (*device)->unmapMemory(memory);
    }

};

// Represents a buffer stores a specific type of object

template<typename T>
class VulkanObjectBuffer : public VulkanBuffer {
protected:

    int count;

public:

    T& operator[](int i) {
        return ((T*) data)[i];
    }

    size_t getObjectCount(void) {
        return count;
    }

    VulkanObjectBuffer(VulkanDevice * device, int n, vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) :
    VulkanBuffer(device, n * sizeof (T), usage, memoryProperties) {
        count = n;
    }

};

// Controls updatable resources
class VulkanUpdateManager {
private:

    std::vector<VulkanUpdatable*> objects;

public:

    void addUpdatable(VulkanUpdatable * obj) {
        objects.push_back(obj);
    }

    void forceUpdate(void * data) {
        for (auto & obj : objects) {
            obj->update(data);
            obj->markUpdated();
        }
    }

    bool updateIfNecessary(void * data) {
        bool updated = false;

        for (auto & obj : objects) {
            updated = updated || (obj->needsUpdate() && obj->modifiesData());
        }

        if (!updated) {
            return false;
        }

        for (auto obj : objects) {
            if (obj->needsUpdate()) {
                obj->update(data);
                obj->markUpdated();
            }
        }

        return true;
    }

};

#endif /* VULKANBUFFER_HPP */

