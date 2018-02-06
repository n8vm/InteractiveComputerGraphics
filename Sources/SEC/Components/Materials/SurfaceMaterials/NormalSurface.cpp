#include "NormalSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	VkShaderModule NormalSurface::vertShaderModule;
	VkShaderModule NormalSurface::fragShaderModule;

	VkPipelineLayout NormalSurface::pipelineLayout;
	VkPipeline NormalSurface::graphicsPipeline;

	VkDescriptorPool NormalSurface::descriptorPool;
	VkDescriptorSetLayout NormalSurface::descriptorSetLayout;
	uint32_t NormalSurface::maxDescriptorSets;
}