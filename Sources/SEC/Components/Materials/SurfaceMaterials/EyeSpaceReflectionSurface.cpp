#include "EyeSpaceReflectionSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	std::vector<EyeSpaceReflectionSurface> EyeSpaceReflectionSurface::EyeSpaceReflectionSurfaces;

	VkShaderModule EyeSpaceReflectionSurface::vertShaderModule;
	VkShaderModule EyeSpaceReflectionSurface::fragShaderModule;

std::unordered_map<VkRenderPass, VkPipelineLayout> EyeSpaceReflectionSurface::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> EyeSpaceReflectionSurface::pipelines;

	VkDescriptorPool EyeSpaceReflectionSurface::descriptorPool;
	VkDescriptorSetLayout EyeSpaceReflectionSurface::descriptorSetLayout;
	uint32_t EyeSpaceReflectionSurface::maxDescriptorSets;
}