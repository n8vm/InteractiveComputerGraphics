#pragma once

#include "vkdk.hpp"
#include "Tools/HashCombiner.hpp"
#include "Systems/ComponentManager.hpp"

struct PipelineKey {
	VkRenderPass renderpass = VK_NULL_HANDLE;
	uint32_t subpass = 0;
	uint32_t pipelineIdx = 0;

	PipelineKey() {}
	PipelineKey(VkRenderPass renderpass, uint32_t subpass, uint32_t pipelineIdx) {
		this->renderpass = renderpass;
		this->subpass = subpass;
		this->pipelineIdx = pipelineIdx;
	}

	bool operator==(const PipelineKey &) const;
};

inline bool PipelineKey::operator==(const PipelineKey  &f) const 
{ 
	return renderpass == f.renderpass && subpass == f.subpass && pipelineIdx == f.pipelineIdx; 
}

template <>
struct std::hash<PipelineKey>
{
	size_t operator()(const PipelineKey& k) const
	{
		std::size_t h = 0;
		hash_combine(h, k.renderpass, k.subpass, k.pipelineIdx);
		return h;
	}

};

struct PipelineParameters {
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	VkPipelineViewportStateCreateInfo viewportState = {};
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	VkPipelineDynamicStateCreateInfo dynamicState = {};

	VkDynamicState dynamicStates[3] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	static std::shared_ptr<PipelineParameters> Create(PipelineKey key) {
		std::cout << "ComponentManager: Registering pipeline settings for renderpass " << key.renderpass
			<< " subpass " << key.subpass << " pipeline index " << key.pipelineIdx << std::endl;
		auto pp = std::make_shared<PipelineParameters>();
		Systems::ComponentManager::PipelineSettings[key] = pp;
		return pp;
	}

	PipelineParameters() {
		/* Default input assembly */
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		/* Default rasterizer */
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // Helpful for shadow maps.
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never goes through rasterization, or to frame buffer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // could be line or point too. those require GPU features to be enabled.
		rasterizer.lineWidth = 1.0f; // anything thicker than 1.0 requires wideLines GPU feature
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		/* Default Viewport and Scissors */
		// Note: This is actually overriden by the dynamic states (see below)
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.pViewports = VK_NULL_HANDLE;
		viewportState.pScissors = VK_NULL_HANDLE;

		/* Default Multisampling */
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		/* Default Depth and Stencil Testing */
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		/* Default Color Blending Attachment State */
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional /* TRANSPARENCY STUFF HERE! */
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		/* Default Color Blending State */
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		/* Default Dynamic State */
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 3;
		dynamicState.pDynamicStates = dynamicStates;
	}
};