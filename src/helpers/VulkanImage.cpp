/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include <stb_image_write.h>

#include <cctype>

#include "VulkanImage.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanSingleCommand.hpp"

Texture::Texture(VulkanDevice & device, std::string path) :
device(device) {
    this->path = path;
    loadImageData(path);
    createImage();
    createImageView();
}

Texture::Texture(VulkanDevice & device, unsigned char * buffer, int width, int height, int channels) :
device(device), width(width), height(height), channels(channels) {
    loadImageData(buffer, width, height);
    createImage();
    createImageView();
}

void Texture::configureLayouts(VulkanCommandBufferPool & pool, VulkanQueue & queue) {
    VulkanSingleCommand::changeImageLayout(device, pool, queue,
            image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal);

    VulkanSingleCommand::copyBufferToImage(device, pool, queue,
            stagingBuffer, image, width, height);

    VulkanSingleCommand::changeImageLayout(device, pool, queue,
            image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::loadImageData(std::string path) {

    stbi_uc * pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Could not load image " + path);
    }
    
    vk::DeviceSize imageSize = width * height * 4;

    device.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer, stagingMemory);

    void* data;
    device->mapMemory(stagingMemory, 0, imageSize, vk::MemoryMapFlags(), &data);
    memcpy(data, pixels, static_cast<size_t> (imageSize));
    device->unmapMemory(stagingMemory);

    stbi_image_free(pixels);
}

void Texture::loadImageData(unsigned char * buffer, int width, int height) {

    if (!buffer) {
        throw std::runtime_error("Buffer cannot be null");
    }

    vk::DeviceSize imageSize = width * height * 4;

    device.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer, stagingMemory);

    void* data;
    device->mapMemory(stagingMemory, 0, imageSize, vk::MemoryMapFlags(), &data);
    
    for(int i = 0; i < width * height; i++) {
        memset(reinterpret_cast<unsigned char*>(data) + i * 4, 0xFF, 4);
        memcpy(reinterpret_cast<unsigned char*>(data) + i * 4, buffer + i * channels, channels);
    }
    
    device->unmapMemory(stagingMemory);
}

void Texture::createImage(void) {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = static_cast<uint32_t> (width);
    imageInfo.extent.height = static_cast<uint32_t> (height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format = vk::Format::eR8G8B8A8Unorm;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    imageInfo.samples = vk::SampleCountFlagBits::e1;

    image = device->createImage(imageInfo);


    vk::MemoryRequirements memRequirements = device->getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    imageMemory = device->allocateMemory(allocInfo);

    if (!imageMemory) {
        throw std::runtime_error("Could not allocate memory for image " + path);
    }

    device->bindImageMemory(image, imageMemory, 0);
}

void Texture::createImageView(void) {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR8G8B8A8Unorm;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    view = device->createImageView(viewInfo);
}

TextureSampler::TextureSampler(VulkanDevice * device,
        int descriptionBinding, vk::ShaderStageFlags shaderStage,
        Texture const * image, vk::Filter magnified, vk::Filter minimized,
        vk::SamplerAddressMode addressMode, int maxAnisotropy,
        vk::BorderColor borderColor) :
VulkanDescriptor(descriptionBinding, 1, vk::DescriptorType::eCombinedImageSampler, shaderStage),
image(image), device(device) {

    vk::SamplerCreateInfo samplerInfo;

    samplerInfo.magFilter = magnified;
    samplerInfo.minFilter = minimized;

    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;

    if (maxAnisotropy == ANISOTROPY_AUTO) {
        bool supported = device->supportsAnisotropy();
        samplerInfo.anisotropyEnable = supported ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = supported ? -(float) ANISOTROPY_AUTO : 1;
    } else {
        samplerInfo.anisotropyEnable = maxAnisotropy != ANISOTROPY_NONE;
        samplerInfo.maxAnisotropy = (float) maxAnisotropy;
    }

    samplerInfo.borderColor = borderColor;

    samplerInfo.unnormalizedCoordinates = false;

    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    sampler = (*device)->createSampler(samplerInfo);
}

void TextureSampler::setTexture(Texture const * image) {
    this->image = image;
    this->markNeedsUpdate();
}

void TextureSampler::update(void* data) {

    if (image == nullptr) {
        throw std::runtime_error("Image cannot be null");
    }

    vk::DescriptorImageInfo imageInfo(sampler, image->getView(), vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::WriteDescriptorSet descWrite;

    descWrite.dstSet = *reinterpret_cast<vk::DescriptorSet*> (data);
    descWrite.descriptorType = this->layoutBinding.descriptorType;
    descWrite.descriptorCount = 1;

    descWrite.dstBinding = layoutBinding.binding;
    descWrite.dstArrayElement = 0;

    descWrite.pImageInfo = &imageInfo;

    (*device)->updateDescriptorSets(1, &descWrite, 0, nullptr);
}

Font::~Font() {
    stbi_image_free(pixels);
}

void Font::loadFontData(const char* filename, bool alpha) {

    pixels = stbi_load(filename, &width, &height, &channels, alpha ? STBI_rgb_alpha : STBI_rgb);

    if (!pixels) {
        throw std::runtime_error("Could not load image " + std::string(filename));
    }

    int current_char = 0;
    int current_index = 0;

    bool character_seen = false;

    // for the top row, check every red channel.
    // If the red channel isn't 255, the pixel is the start of a new character
    for (int i = 0; i < width * channels; i++) {

        unsigned char chr = pixels[i] & 0xFF;

        if (chr != 0xFF && i % channels == 0) {
            int index = i / channels;

            if (character_seen) {
                characters.push_back(Character{current_char, current_index, index - current_index});
            }

            current_index = index;
            current_char = chr;
            character_seen = true;
        }
    }
}

std::shared_ptr<unsigned char> Font::RenderImage(char const * text, int * _width, int * _height, int * _channels) {
    int width = 0;

    std::vector<struct Character*> chars;

    // convert the string to the character vector and calculate the image width
    for (char const * curr = text; *curr; curr++) {
        try {
            int c = std::tolower(*curr);
            for (auto & ch : characters) {
                if (ch.c == c) {
                    width += ch.width;
                    chars.push_back((&ch));
                }
            }
        } catch (std::out_of_range) {
            throw std::runtime_error("Unknown character '" + std::string(1, *curr) + "'");
        }
    }
    
    size_t n = width * (height - 1) * channels;
    std::shared_ptr<unsigned char> pixels(new unsigned char[n]);

    memset(pixels.get(), 0, n);

    int x = 0;

    for (auto & ch : chars) {
        for (int i = 0; i < height-1; i++) {
            unsigned char * src = &this->pixels[channels * (this->width * (i+1) + ch->xoffset)];
            unsigned char * dst = &pixels.get()[channels * (width * i + x)];
            memcpy(dst, src, channels * ch->width);
        }

        x += ch->width;
    }

    *_width = width;
    *_height = height - 1;
    *_channels = channels;

    return pixels;
}
