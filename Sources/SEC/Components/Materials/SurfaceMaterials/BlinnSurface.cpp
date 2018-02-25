#include "BlinnSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<BlinnSurface> BlinnSurface::BlinnSurfaces;

	VkShaderModule BlinnSurface::vertShaderModule;
	VkShaderModule BlinnSurface::fragShaderModule;

	VkPipelineLayout BlinnSurface::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> BlinnSurface::graphicsPipelines;

	VkDescriptorPool BlinnSurface::descriptorPool;
	VkDescriptorSetLayout BlinnSurface::descriptorSetLayout;
	uint32_t BlinnSurface::maxDescriptorSets;
}