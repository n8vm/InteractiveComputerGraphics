#include "TexturedBlinnSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<TexturedBlinnSurface> TexturedBlinnSurface::TexturedBlinnSurfaces;

	VkShaderModule TexturedBlinnSurface::vertShaderModule;
	VkShaderModule TexturedBlinnSurface::fragShaderModule;

	VkPipelineLayout TexturedBlinnSurface::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> TexturedBlinnSurface::graphicsPipelines;

	VkDescriptorPool TexturedBlinnSurface::descriptorPool;
	VkDescriptorSetLayout TexturedBlinnSurface::descriptorSetLayout;
	uint32_t TexturedBlinnSurface::maxDescriptorSets;
}