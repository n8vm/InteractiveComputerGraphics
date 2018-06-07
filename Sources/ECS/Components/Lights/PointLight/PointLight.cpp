#include "PointLight.hpp"

namespace Components::Lights{
	VkBuffer PointLights::pointLightUBO;
	VkDeviceMemory PointLights::pointLightUBOMemory;
}