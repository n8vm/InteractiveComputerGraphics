#include "BlinnShadowSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<BlinnShadowSurface> BlinnShadowSurface::BlinnShadowSurfaces;

	VkShaderModule BlinnShadowSurface::vertShaderModule;
	VkShaderModule BlinnShadowSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> BlinnShadowSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> BlinnShadowSurface::pipelines;

	VkDescriptorPool BlinnShadowSurface::descriptorPool;
	VkDescriptorSetLayout BlinnShadowSurface::descriptorSetLayout;
	uint32_t BlinnShadowSurface::maxDescriptorSets;
}