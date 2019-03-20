/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VulkanImage.hpp
 * Author: austin-z
 *
 * Created on February 9, 2019, 12:13 PM
 */

#ifndef VULKANIMAGE_HPP
#define VULKANIMAGE_HPP

#include "VulkanDevice.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanRenderPipeline.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

#include <string>
#include <fstream>
#include <vector>

class Texture {
private:

    int width = 0, height = 0, channels = 0;

    vk::Image image;
    vk::ImageView view;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory, imageMemory;

    VulkanDevice & device;

	std::string path;

public:

    Texture(VulkanDevice & device, std::string path);

	Texture(VulkanDevice & device, unsigned char * buffer, int width, int height);

	Texture(const Texture & other) = delete;

	~Texture() {
		device->destroyImageView(view);
		device->destroyImage(image);
		device->freeMemory(imageMemory);
		device->destroyBuffer(stagingBuffer);
		device->freeMemory(stagingMemory);
	}

    void configureLayouts(VulkanCommandBufferPool & pool, VulkanQueue & queue);

    vk::Image getImage(void) {
        return image;
    }

    vk::ImageView getView(void) const{
        return view;
    }

	int getWidth() {
		return width;
	}

	int getHeight() {
		return height;
	}

private:

    void loadImageData(std::string path);

	void loadImageData(unsigned char * buffer, int width, int height);

    void createImage();

    void createImageView();

};

class TextureSampler : public VulkanDescriptor {
private:

    Texture const * image = nullptr;

    vk::Sampler sampler;

    VulkanDevice * device = nullptr;

public:

    static constexpr int ANISOTROPY_AUTO = -16, ANISOTROPY_NONE = 1;

    TextureSampler(VulkanDevice * device, int descriptionBinding,
            vk::ShaderStageFlags shaderStage, Texture const * image, 
			vk::Filter magnified = vk::Filter::eLinear,
            vk::Filter minimized = vk::Filter::eLinear,
            vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat,
            int maxAnisotropy = ANISOTROPY_NONE,
            vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack);

	void setTexture(Texture const * image);

protected:
    void update(void* data) override;

    bool modifiesData() override {
        return true;
    }
};

class ImageManager {
private:
	std::map<std::string, Texture*> images;

	VulkanDevice * device;
	VulkanCommandBufferPool * pool;
	VulkanQueue * queue;

public:

	ImageManager(VulkanDevice * device, VulkanCommandBufferPool * pool, VulkanQueue * queue):
	device(device), pool(pool), queue(queue) {
	}

	Texture const * getImage(std::string path) {
		try {
			return images.at(path);
		}
		catch (std::out_of_range oor) {
			Texture * image = new Texture(*device, path);
			images[path] = image;

			image->configureLayouts(*pool, *queue);

			return image;
		}
	}

};

struct TextureSamplerPrototype {
	int id;
	Texture const * texture;
	int binding;
	vk::ShaderStageFlags stages;
	vk::Filter magnified = vk::Filter::eLinear;
	vk::Filter minimized = vk::Filter::eLinear;
	vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
	int maxAnisotropy = TextureSampler::ANISOTROPY_NONE;
	vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack;

	TextureSamplerPrototype(int id, Texture const * texture, int binding, vk::ShaderStageFlags stages) :
		id(id), texture(texture), binding(binding), stages(stages) {

	}
};

struct UniformBufferPrototype {
	int id = 0;
	int binding = 0;
	size_t elSize = 0;
	int count = 0;
	vk::ShaderStageFlags stages;

	UniformBufferPrototype() {

	}

	UniformBufferPrototype(int id, int binding, size_t elSize, int count, vk::ShaderStageFlags stages) :
		id(id), binding(binding), elSize(elSize), count(count), stages(stages) {

	}
};

struct VertexDescriptorInfo {
	VulkanVertexDescriptor * desc;
	VulkanBuffer * buffer;
};

struct ShaderPrototype {
	std::string path;
	vk::ShaderStageFlagBits stages;
};

struct PushConstantPrototype {
	int id;
	int offset, size;
	vk::ShaderStageFlags stages;
};

struct PushConstant {
	int id = 0;
	std::shared_ptr<char> ptr;
	int offset = 0;
	int size = 0;
	vk::ShaderStageFlags stages;

	PushConstant() {

	}

	PushConstant(struct PushConstantPrototype prototype):
			ptr(new char[prototype.size]) {
		this->id = prototype.id;
		this->offset = prototype.offset;
		this->size = prototype.size;
		this->stages = prototype.stages;
	}
};

struct MaterialPrototype {
	std::vector<struct TextureSamplerPrototype> samplers;
	std::vector<struct UniformBufferPrototype> buffers;
	std::vector<struct VertexDescriptorInfo> vertexDescriptors;
	std::vector<struct ShaderPrototype> shaders;
	std::vector<struct PushConstantPrototype> pushConstants;
};

using UBOMap = std::map<int, std::unique_ptr<VulkanUniformBuffer>>;

class Material {
private:

	struct MaterialInfo {
		std::map<int, std::unique_ptr<TextureSampler>> samplers;
		UBOMap buffers;
		std::vector<struct VertexDescriptorInfo> vertexDescriptors;
		std::vector<std::unique_ptr<VulkanShader>> shaders;
		std::map<int, struct PushConstant> pushConstants;

		std::unique_ptr<VulkanPipeline> pipeline;
		std::unique_ptr<VulkanDescriptorManager> descriptorManager;
		VulkanUpdateManager updater;

		UBOMap * gbuffers = nullptr;
	};

	std::shared_ptr<struct MaterialInfo> info;

public:

	Material(VulkanDevice & owner, MaterialPrototype & prototype, VulkanViewport & viewport,
			VulkanRenderPass & renderPass, VulkanQueue & queue, UBOMap * globals = nullptr):
		info(new MaterialInfo()){

		createSamplers(owner, prototype.samplers);
		createBuffers(owner, prototype.buffers);
		createShaders(owner, prototype.shaders);

		for (auto & pcProto : prototype.pushConstants) {
			info->pushConstants[pcProto.id] = pcProto;
		}

		info->vertexDescriptors = prototype.vertexDescriptors;
		info->gbuffers = globals;

		createDescriptorSet(owner);
		createPipeline(owner, viewport, renderPass, queue);
	}

	VulkanUniformBuffer * getBuffer(int id) {
		try {
			return info->buffers.at(id).get();
		}
		catch (std::out_of_range) {
			throw std::runtime_error("Cannot find buffer with provided id");
		}
	}

	TextureSampler * getSampler(int id) {
		try {
			return info->samplers.at(id).get();
		}
		catch (std::out_of_range) {
			throw std::runtime_error("Cannot find sampler with provided id");
		}
	}

	std::shared_ptr<char> pushconst(int id) {
		return info->pushConstants.at(id).ptr;
	}

	std::vector<vk::Buffer> getVertexBuffers(void) {
		std::vector<vk::Buffer> buffers;
		for (auto & v : info->vertexDescriptors) {
			buffers.push_back(v.buffer->getBuffer());
		}
		return buffers;
	}

	const vk::DescriptorSet & getDescriptorSet(void) const {
		return info->descriptorManager->getSet();
	}

	VulkanPipeline * getPipeline(void) {
		return info->pipeline.get();
	}

	const std::map<int, struct PushConstant> & getPushConstants(void) const {
		return info->pushConstants;
	}

	bool hasDescriptors(void) {
		return info->descriptorManager->count() > 0;
	}

	bool hasPushConstants(void) {
		return info->pushConstants.size() > 0;
	}

	bool checkForUpdates(bool force = false) {
		if (force) {
			info->updater.forceUpdate(&info->descriptorManager->getSet());
			return true;
		}
		else {
			return info->updater.updateIfNecessary(&info->descriptorManager->getSet());
		}
	}

	static UBOMap createubos(VulkanDevice & owner,
		std::vector<struct UniformBufferPrototype> && prototypes) {
        return createubos(owner, prototypes);
    }
    
	static UBOMap createubos(VulkanDevice & owner,
		std::vector<struct UniformBufferPrototype> & prototypes) {

		UBOMap buffers;

		for (size_t i = 0; i < prototypes.size(); i++) {
			struct UniformBufferPrototype * proto = &prototypes[i];
			buffers[proto->id] = std::move(std::unique_ptr<VulkanUniformBuffer>(
				new VulkanUniformBuffer(&owner, proto->binding, proto->elSize, proto->count, proto->stages)));
		}

		return buffers;
	}

private:

	void createSamplers(VulkanDevice & owner, std::vector<struct TextureSamplerPrototype> & prototypes) {
		
		for (size_t i = 0; i < prototypes.size(); i++) {
			struct TextureSamplerPrototype * proto = &prototypes[i];
			this->info->samplers[proto->id] = std::move(std::unique_ptr<TextureSampler>(
				new TextureSampler(&owner, proto->binding, proto->stages,
				proto->texture, proto->magnified, proto->minimized, proto->addressMode, proto->maxAnisotropy,
				proto->borderColor)));
		}

	}

	void createBuffers(VulkanDevice & owner, std::vector<struct UniformBufferPrototype> & prototypes) {
		this->info->buffers = Material::createubos(owner, prototypes);
	}

	void createShaders(VulkanDevice & owner, std::vector<struct ShaderPrototype> & prototypes) {
		this->info->shaders.resize(prototypes.size());

		for (size_t i = 0; i < prototypes.size(); i++) {
			struct ShaderPrototype * proto = &prototypes[i];
			this->info->shaders[i] = std::move(std::unique_ptr<VulkanShader>(
				new VulkanShader(owner, proto->path, proto->stages)));
		}

	}
	
	void createDescriptorSet(VulkanDevice & owner) {
		info->descriptorManager = std::move(std::unique_ptr<VulkanDescriptorManager>(new VulkanDescriptorManager(owner)));


		for (auto & sampler : info->samplers) {
			info->updater.addUpdatable(sampler.second.get());
			info->descriptorManager->addDescriptor(sampler.second.get());
		}

		for (auto & buffer : info->buffers) {
			info->updater.addUpdatable(buffer.second.get());
			info->descriptorManager->addDescriptor(buffer.second.get());
		}

		for (auto & gbuffer : *info->gbuffers) {
			info->updater.addUpdatable(gbuffer.second.get());
			info->descriptorManager->addDescriptor(gbuffer.second.get());
		}

		for (auto & vertexbuffer : info->vertexDescriptors) {
			info->updater.addUpdatable(vertexbuffer.buffer);
		}

		info->descriptorManager->finalize();
	}

	void createPipeline(VulkanDevice & owner, VulkanViewport & viewport,
		VulkanRenderPass & renderPass, VulkanQueue & queue) {
		std::vector<vk::DescriptorSetLayout> sets{ info->descriptorManager->getLayout() };

		VulkanVertexInputState vertexInput;

		for (auto & vertDescriptor : info->vertexDescriptors) {
			vertexInput.addDescriptor(vertDescriptor.desc);
		}

		std::vector<vk::PushConstantRange> ranges;

		for (auto & pc : info->pushConstants) {
			ranges.push_back(vk::PushConstantRange(pc.second.stages, pc.second.offset, pc.second.size));
		}

		info->pipeline = std::move(std::unique_ptr<VulkanPipeline>(
			VulkanPipelineFactory::create(owner, viewport, renderPass,
				sets, vertexInput, info->shaders, ranges)));
	}

};

class MaterialPrototypeHelpers {
public:

	static void AddSamplerToMany(std::vector<struct MaterialPrototype*> & prototypes,
		const struct TextureSamplerPrototype & sampler) {
		for (auto & mat : prototypes) {
			mat->samplers.push_back(sampler);
		}
	}

	static void ZipSamplersToMaterials(std::vector<struct MaterialPrototype*> & prototypes,
		std::vector<struct TextureSamplerPrototype> & samplers) {
		if (prototypes.size() != samplers.size()) {
			throw std::runtime_error("prototype and sampler vectors must be the same size");
		}

		for (size_t i = 0; i < prototypes.size(); i++) {
			prototypes[i]->samplers.push_back(samplers[i]);
		}
	}

	static void AddBufferToMany(std::vector<struct MaterialPrototype*> & prototypes,
		const struct UniformBufferPrototype & buffer) {
		for (auto & mat : prototypes) {
			mat->buffers.push_back(buffer);
		}
	}

	static void AddVertexDescriptorToMany(std::vector<struct MaterialPrototype*> & prototypes,
		const struct VertexDescriptorInfo & vdesc) {
		for (auto & mat : prototypes) {
			mat->vertexDescriptors.push_back(vdesc);
		}
	}

	static void AddShaderToMany(std::vector<struct MaterialPrototype*> & prototypes,
		const struct ShaderPrototype & shader) {
		for (auto & mat : prototypes) {
			mat->shaders.push_back(shader);
		}
	}

	static void AddPushConstantToMany(std::vector<struct MaterialPrototype*> & prototypes,
		const struct PushConstantPrototype & pc) {
		for (auto & mat : prototypes) {
			mat->pushConstants.push_back(pc);
		}
	}

};

class Font {
private:
	std::vector<unsigned char> font_data;

	float ascent = 0, descent = 0, lineGap = 0, scale = 0, baseline = 0, height = 0;

	stbtt_fontinfo font = { 0 };

public:
	
	Font(const char * filename, float size, int index = 0);

	std::vector<unsigned char> RenderImage(char const * text, int * _width, int * _height);

private:

	void loadFontData(const char * filename) {
		std::ifstream in(filename, std::ifstream::binary | std::ifstream::ate);

		font_data.resize((size_t)in.tellg());

		in.seekg(std::ifstream::beg);

		in.read((char*)font_data.data(), font_data.size());

		in.close();
	}

};

class TextImage {
private:

	Font * font;
	char const * text = nullptr;

	Texture * texture = nullptr;
	TextureSampler * sampler;

	int width, height;

public:
	TextImage(VulkanDevice & device, TextureSampler * sampler, Font * font, char const * text = nullptr) {
		this->font = font;
		this->sampler = sampler;
		if (text) {
			setText(device, text);
		}
	}

	void setText(VulkanDevice & device, char const * text) {
		this->text = text;
		if (text == nullptr) {
			throw std::runtime_error("Text cannot be null");
		}

		if (texture) {
			delete texture;
		}

		std::vector<unsigned char> image = font->RenderImage(text, &width, &height);

		texture = new Texture(device, image.data(), width, height);

		sampler->setTexture(texture);
	}

	int getWidth() {
		return width;
	}

	int getHeight() {
		return height;
	}

};

#endif /* VULKANIMAGE_HPP */
