#include "TexturedBlinnSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<TexturedBlinnSurface> TexturedBlinnSurface::TexturedBlinnSurfaces;

	VkShaderModule TexturedBlinnSurface::vertShaderModule;
	VkShaderModule TexturedBlinnSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> TexturedBlinnSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> TexturedBlinnSurface::pipelines;

	VkDescriptorPool TexturedBlinnSurface::descriptorPool;
	VkDescriptorSetLayout TexturedBlinnSurface::descriptorSetLayout;
	uint32_t TexturedBlinnSurface::maxDescriptorSets;
}