#include "NormalSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<NormalSurface> NormalSurface::NormalSurfaces;

	VkShaderModule NormalSurface::vertShaderModule;
	VkShaderModule NormalSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> NormalSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> NormalSurface::pipelines;

	VkDescriptorPool NormalSurface::descriptorPool;
	VkDescriptorSetLayout NormalSurface::descriptorSetLayout;
	uint32_t NormalSurface::maxDescriptorSets;
}