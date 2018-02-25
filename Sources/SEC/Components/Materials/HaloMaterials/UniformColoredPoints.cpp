#include "UniformColoredPoints.hpp"

namespace Components::Materials::HaloMaterials {
	std::vector<UniformColoredPoint> UniformColoredPoint::UniformColoredPoints;
	
	VkShaderModule UniformColoredPoint::vertShaderModule;
	VkShaderModule UniformColoredPoint::fragShaderModule;

	VkPipelineLayout UniformColoredPoint::pipelineLayout;
	std::unordered_map<VkRenderPass, VkPipeline> UniformColoredPoint::graphicsPipelines;

	VkDescriptorPool UniformColoredPoint::descriptorPool;
	VkDescriptorSetLayout UniformColoredPoint::descriptorSetLayout;
	uint32_t UniformColoredPoint::maxDescriptorSets;
}