#include "SkyboxSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<SkyboxSurface> SkyboxSurface::SkyboxSurfaces;

	VkShaderModule SkyboxSurface::vertShaderModule;
	VkShaderModule SkyboxSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> SkyboxSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> SkyboxSurface::pipelines;

	VkDescriptorPool SkyboxSurface::descriptorPool;
	VkDescriptorSetLayout SkyboxSurface::descriptorSetLayout;
	uint32_t SkyboxSurface::maxDescriptorSets;
}