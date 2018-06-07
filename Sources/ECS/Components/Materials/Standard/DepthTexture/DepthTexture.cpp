#include "DepthTextureSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<DepthTextureSurface> DepthTextureSurface::DepthTextureSurfaces;

	VkShaderModule DepthTextureSurface::vertShaderModule;
	VkShaderModule DepthTextureSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> DepthTextureSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> DepthTextureSurface::pipelines;

	VkDescriptorPool DepthTextureSurface::descriptorPool;
	VkDescriptorSetLayout DepthTextureSurface::descriptorSetLayout;
	uint32_t DepthTextureSurface::maxDescriptorSets;
}