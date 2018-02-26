#include "UniformColoredSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<UniformColoredSurface> UniformColoredSurface::UniformColoredSurfaces;

	VkShaderModule UniformColoredSurface::vertShaderModule;
	VkShaderModule UniformColoredSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> UniformColoredSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> UniformColoredSurface::pipelines;

	VkDescriptorPool UniformColoredSurface::descriptorPool;
	VkDescriptorSetLayout UniformColoredSurface::descriptorSetLayout;
	uint32_t UniformColoredSurface::maxDescriptorSets;
}