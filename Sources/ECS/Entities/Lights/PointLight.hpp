#pragma once
#include "Entities/Lights/Light.hpp"

namespace Entities::Lights {
	class PointLight : public Light {
		public: 
		PointLight(glm::vec3 diffuseColor, float diffusePower, glm::vec3 specularColor, float specularPower) : Light () {
			// struct lightSource
			// {
			//   vec4 position;
			//   vec4 diffuse;
			//   vec4 specular;
			//   float constantAttenuation, linearAttenuation, quadraticAttenuation;
			//   float spotCutoff, spotExponent;
			//   vec3 spotDirection;
			// };
		}


	};
}