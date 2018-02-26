#include "TexCoordSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<TexCoordSurface> TexCoordSurface::TexCoordSurfaces;

	VkShaderModule TexCoordSurface::vertShaderModule;
	VkShaderModule TexCoordSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> TexCoordSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> TexCoordSurface::pipelines;

	VkDescriptorPool TexCoordSurface::descriptorPool;
	VkDescriptorSetLayout TexCoordSurface::descriptorSetLayout;
	uint32_t TexCoordSurface::maxDescriptorSets;
}