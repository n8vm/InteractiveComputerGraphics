#include "TexCoordSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<TexCoordSurface> TexCoordSurface::TexCoordSurfaces;

	VkShaderModule TexCoordSurface::vertShaderModule;
	VkShaderModule TexCoordSurface::fragShaderModule;

	VkPipelineLayout TexCoordSurface::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> TexCoordSurface::graphicsPipelines;

	VkDescriptorPool TexCoordSurface::descriptorPool;
	VkDescriptorSetLayout TexCoordSurface::descriptorSetLayout;
	uint32_t TexCoordSurface::maxDescriptorSets;
}