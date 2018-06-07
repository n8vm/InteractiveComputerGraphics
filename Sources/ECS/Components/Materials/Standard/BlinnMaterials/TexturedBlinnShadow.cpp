#include "TexturedBlinnShadowSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<TexturedBlinnShadowSurface> TexturedBlinnShadowSurface::TexturedBlinnShadowSurfaces;

	VkShaderModule TexturedBlinnShadowSurface::vertShaderModule;
	VkShaderModule TexturedBlinnShadowSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> TexturedBlinnShadowSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> TexturedBlinnShadowSurface::pipelines;

	VkDescriptorPool TexturedBlinnShadowSurface::descriptorPool;
	VkDescriptorSetLayout TexturedBlinnShadowSurface::descriptorSetLayout;
	uint32_t TexturedBlinnShadowSurface::maxDescriptorSets;
}