/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

Texture::Texture(VulkanDevice & device, unsigned char * buffer, int width, int height) :
device(device) {
	this->path = nullptr;
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
	memcpy(data, buffer, static_cast<size_t> (imageSize));
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

Font::Font(const char * filename, float size, int index) {
	loadFontData(filename);

	int nfonts = stbtt_GetNumberOfFonts(font_data.data());

	if (nfonts == -1) {
		throw std::runtime_error("Could not load font count");
	}

	if (nfonts == 0) {
		throw std::runtime_error("Font file has no fonts");
	}

	if (index < 0 || index >= nfonts) {
		throw std::out_of_range("Index is outside of valid index range");
	}

	stbtt_GetScaledFontVMetrics(font_data.data(), index, size, &ascent, &descent, &lineGap);
	if (stbtt_InitFont(&font, font_data.data(), stbtt_GetFontOffsetForIndex(font_data.data(), index)) == 0) {
		throw std::runtime_error("Could not load font");
	}

	scale = stbtt_ScaleForPixelHeight(&font, size);
	baseline = ascent*scale;
	height = (ascent - descent)*scale;

}

std::vector<unsigned char> Font::RenderImage(char const * text, int * _width, int * _height) {
	int width = 0;

	// calculate the image's width
	for (char const * ptr = text; *ptr; ptr++) {
		/* how wide is this character */
		int ax;
		stbtt_GetCodepointHMetrics(&font, *ptr, &ax, nullptr);
		width += (int)(ax * scale);

		/* add kerning */
		int kern;
		kern = stbtt_GetCodepointKernAdvance(&font, *ptr, *(ptr + 1));
		width += (int)(kern * scale);
	}

	std::vector<unsigned char> pixels((int)(width * height * 4));

	// render the text to the image
	int x = 0;
	for (char const * ptr = text; *ptr; ptr++) {
		/* get bounding box for character (may be offset to account for chars that dip above or below the line */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox(&font, *ptr, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

		/* compute y (different characters have different heights */
		int y = (int)(ascent + c_y1);

		/* render character (stride and offset is important here) */
		int byteOffset = x + (y  * width);
		stbtt_MakeCodepointBitmap(&font, &pixels[byteOffset], c_x2 - c_x1, c_y2 - c_y1, width, scale, scale, *ptr);

		/* how wide is this character */
		int ax;
		stbtt_GetCodepointHMetrics(&font, *ptr, &ax, 0);
		x += (int)(ax * scale);

		/* add kerning */
		int kern;
		kern = stbtt_GetCodepointKernAdvance(&font, *ptr, *(ptr + 1));
		x += (int)(kern * scale);
	}

	if (_width) {
		*_width = width;
	}

	if (_height) {
		*_height = (int)height;
	}

	return pixels;
}
