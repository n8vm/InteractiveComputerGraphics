#include "UniformColoredSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	VkShaderModule UniformColoredSurface::vertShaderModule;
	VkShaderModule UniformColoredSurface::fragShaderModule;

	VkPipelineLayout UniformColoredSurface::pipelineLayout;
	VkPipeline UniformColoredSurface::graphicsPipeline;

	VkDescriptorPool UniformColoredSurface::descriptorPool;
	VkDescriptorSetLayout UniformColoredSurface::descriptorSetLayout;
	uint32_t UniformColoredSurface::maxDescriptorSets;
}