#include "BlinnSurface.hpp"

namespace Components::Materials::SurfaceMaterials {
	VkShaderModule BlinnSurface::vertShaderModule;
	VkShaderModule BlinnSurface::fragShaderModule;

	VkPipelineLayout BlinnSurface::pipelineLayout;
	VkPipeline BlinnSurface::graphicsPipeline;

	VkDescriptorPool BlinnSurface::descriptorPool;
	VkDescriptorSetLayout BlinnSurface::descriptorSetLayout;
	uint32_t BlinnSurface::maxDescriptorSets;
}