#include "BlinnSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<BlinnSurface> BlinnSurface::BlinnSurfaces;

	VkShaderModule BlinnSurface::vertShaderModule;
	VkShaderModule BlinnSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> BlinnSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> BlinnSurface::pipelines;

	VkDescriptorPool BlinnSurface::descriptorPool;
	VkDescriptorSetLayout BlinnSurface::descriptorSetLayout;
	uint32_t BlinnSurface::maxDescriptorSets;
}