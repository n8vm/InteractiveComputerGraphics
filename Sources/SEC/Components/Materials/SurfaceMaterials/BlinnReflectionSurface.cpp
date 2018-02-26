#include "BlinnReflectionSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<BlinnReflectionSurface> BlinnReflectionSurface::BlinnReflectionSurfaces;

	VkShaderModule BlinnReflectionSurface::vertShaderModule;
	VkShaderModule BlinnReflectionSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> BlinnReflectionSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> BlinnReflectionSurface::pipelines;

	VkDescriptorPool BlinnReflectionSurface::descriptorPool;
	VkDescriptorSetLayout BlinnReflectionSurface::descriptorSetLayout;
	uint32_t BlinnReflectionSurface::maxDescriptorSets;
}