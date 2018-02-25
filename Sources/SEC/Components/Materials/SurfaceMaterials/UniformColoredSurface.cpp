#include "UniformColoredSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<UniformColoredSurface> UniformColoredSurface::UniformColoredSurfaces;

	VkShaderModule UniformColoredSurface::vertShaderModule;
	VkShaderModule UniformColoredSurface::fragShaderModule;

	VkPipelineLayout UniformColoredSurface::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> UniformColoredSurface::graphicsPipelines;

	VkDescriptorPool UniformColoredSurface::descriptorPool;
	VkDescriptorSetLayout UniformColoredSurface::descriptorSetLayout;
	uint32_t UniformColoredSurface::maxDescriptorSets;
}