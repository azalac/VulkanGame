
#ifndef _VULKAN_RENDER_PIPELINE
#define _VULKAN_RENDER_PIPELINE

#include "VulkanShader.hpp"
#include "VulkanDescriptor.hpp"

#include "VulkanRenderPass.hpp"
#include "VulkanCommandBuffer.hpp"

#include "VulkanVertex.hpp"

#include <vector>

class VulkanPipeline {
private:

    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::PipelineCache cache;

public:

    VulkanPipeline(VulkanDevice & device, std::vector<vk::PushConstantRange> & pushConstants,
			std::vector<vk::DescriptorSetLayout> & sets, vk::GraphicsPipelineCreateInfo & pipelineInfo);

    VulkanPipeline(const VulkanPipeline & rhs) = delete;

    vk::Pipeline & operator->(void) {
        return pipeline.get();
    }

    operator vk::Pipeline&() {
        return pipeline.get();
    }

	vk::Pipeline get(void) {
		return pipeline.get();
	}

    vk::PipelineLayout & getLayout() {
        return pipelineLayout.get();
    }

private:

    void createLayout(VulkanDevice & device, std::vector<vk::DescriptorSetLayout> & sets,
            std::vector<vk::PushConstantRange> & pushConstants);

};

class VulkanPipelineFactory {

public:
    static VulkanPipeline * create(VulkanDevice & device, VulkanViewport & viewport,
			VulkanRenderPass & renderPass, std::vector<vk::DescriptorSetLayout> & sets,
            VulkanVertexInputState & vertexInput,
			std::vector<std::unique_ptr<VulkanShader>> & shaders,
            std::vector<vk::PushConstantRange> & pushConstants);

};

#endif /* _VULKAN_RENDER_PIPELINE */