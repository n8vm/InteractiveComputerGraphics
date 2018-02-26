#include "UniformColoredPoints.hpp"

namespace Components::Materials::HaloMaterials {
	std::vector<UniformColoredPoint> UniformColoredPoint::UniformColoredPoints;
	
	VkShaderModule UniformColoredPoint::vertShaderModule;
	VkShaderModule UniformColoredPoint::fragShaderModule;

	std::unordered_map<VkRenderPass, VkPipelineLayout> UniformColoredPoint::pipelineLayouts;
	std::unordered_map<VkRenderPass, VkPipeline> UniformColoredPoint::pipelines;

	VkDescriptorPool UniformColoredPoint::descriptorPool;
	VkDescriptorSetLayout UniformColoredPoint::descriptorSetLayout;
	uint32_t UniformColoredPoint::maxDescriptorSets;
}