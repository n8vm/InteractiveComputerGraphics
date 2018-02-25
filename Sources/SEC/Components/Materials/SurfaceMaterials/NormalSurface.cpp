#include "NormalSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<NormalSurface> NormalSurface::NormalSurfaces;

	VkShaderModule NormalSurface::vertShaderModule;
	VkShaderModule NormalSurface::fragShaderModule;

	VkPipelineLayout NormalSurface::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> NormalSurface::graphicsPipelines;

	VkDescriptorPool NormalSurface::descriptorPool;
	VkDescriptorSetLayout NormalSurface::descriptorSetLayout;
	uint32_t NormalSurface::maxDescriptorSets;
}