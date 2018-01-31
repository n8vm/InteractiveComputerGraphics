#include "UniformColoredPoints.hpp"

namespace Components::Materials::HaloMaterials {
	VkShaderModule UniformColoredPoints::vertShaderModule;
	VkShaderModule UniformColoredPoints::fragShaderModule;

	VkPipelineLayout UniformColoredPoints::pipelineLayout;
	VkPipeline UniformColoredPoints::graphicsPipeline;

	VkDescriptorPool UniformColoredPoints::descriptorPool;
	VkDescriptorSetLayout UniformColoredPoints::descriptorSetLayout;
	uint32_t UniformColoredPoints::maxDescriptorSets;
}