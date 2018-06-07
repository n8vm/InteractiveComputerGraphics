#include "CubemapReflectionSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<CubemapReflectionSurface> CubemapReflectionSurface::CubemapReflectionSurfaces;

	VkShaderModule CubemapReflectionSurface::vertShaderModule;
	VkShaderModule CubemapReflectionSurface::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> CubemapReflectionSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> CubemapReflectionSurface::pipelines;

	VkDescriptorPool CubemapReflectionSurface::descriptorPool;
	VkDescriptorSetLayout CubemapReflectionSurface::descriptorSetLayout;
	uint32_t CubemapReflectionSurface::maxDescriptorSets;
}