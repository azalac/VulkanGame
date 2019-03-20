/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "VulkanRenderPipeline.hpp"
#include "VulkanSingleCommand.hpp"

VulkanPipeline::VulkanPipeline(VulkanDevice & device, std::vector<vk::PushConstantRange> & pushConstants,
		std::vector<vk::DescriptorSetLayout> & sets, vk::GraphicsPipelineCreateInfo & pipelineInfo) {

    createLayout(device, sets, pushConstants);

    pipelineInfo.layout = pipelineLayout.get();

    cache = device->createPipelineCache(vk::PipelineCacheCreateInfo());

    pipeline = device->createGraphicsPipelineUnique(cache, pipelineInfo);
}

void VulkanPipeline::createLayout(VulkanDevice & device,
        std::vector<vk::DescriptorSetLayout> & sets,
        std::vector<vk::PushConstantRange> & pushConstants) {
    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.pushConstantRangeCount = pushConstants.size();
    createInfo.pPushConstantRanges = pushConstants.data();

    createInfo.setLayoutCount = sets.size();
    createInfo.pSetLayouts = sets.data();

    pipelineLayout = device->createPipelineLayoutUnique(createInfo);

    if (!pipelineLayout) {
        throw std::runtime_error("Could not create pipeline layout");
    }

}

VulkanPipeline * VulkanPipelineFactory::create(VulkanDevice & device, VulkanViewport & viewport,
        VulkanRenderPass & renderPass, std::vector<vk::DescriptorSetLayout> & sets,
        VulkanVertexInputState & vertexInput, std::vector<std::unique_ptr<VulkanShader>> & shaders,
        std::vector<vk::PushConstantRange> & pushConstants) {

    // STATIC SHADER MODULES START

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vertexInput;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.lineWidth = 1;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = false; // Optional
    multisampling.alphaToOneEnable = false; // Optional

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = true;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = false;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

	vk::DynamicState dStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamicState(vk::PipelineDynamicStateCreateFlags(), sizeof(dStates) / sizeof(*dStates), dStates);

    // STATIC SHADER MODULES END

    vk::GraphicsPipelineCreateInfo pipelineInfo;

	vk::PipelineViewportStateCreateInfo viewportstate = viewport.getViewportState();

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportstate;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState; // Optional

    std::vector<vk::PipelineShaderStageCreateInfo> shader_data;

    for (auto & shader : shaders) {
        shader_data.push_back(*shader);
    }

    pipelineInfo.stageCount = shader_data.size();
    pipelineInfo.pStages = shader_data.data();

    pipelineInfo.renderPass = renderPass.getRenderPass();
    pipelineInfo.subpass = 0;

    return new VulkanPipeline(device, pushConstants, sets, pipelineInfo);
}
