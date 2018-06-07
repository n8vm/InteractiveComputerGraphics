#pragma once

#include "vkdk.hpp"

namespace Components::Materials { class MaterialInterface; }

/* Struct to bundle common static material properties */
struct MaterialProperties {
	VkPipelineLayout pipelineLayout;
	std::unordered_map<PipelineKey, VkPipeline> pipelines;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	uint32_t maxDescriptorSets;
	std::unordered_map<size_t, VkDescriptorSet> descriptorSets;
	//std::vector<Components::Materials::MaterialInterface> instances;
};