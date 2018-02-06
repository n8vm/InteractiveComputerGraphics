#pragma once
#include "Entities/Entity.hpp"
#include "Entities/Model/Model.hpp"

namespace Entities::Lights {
	/* A light block is a struct which contains information about a particular light to use. */
	struct LightObject { /* TODO: maybe change this name */
		glm::vec4 position;
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		//float ambientContribution, constantAttenuation, linearAttenuation, quadraticAttenuation;
		//float spotCutoff, spotExponent;
		//glm::vec3 spotDirection;
	};

#define MAXLIGHTS 3
	struct LightBufferObject {
		LightObject lights[MAXLIGHTS] = {};
		int numLights = 0;
	};

	/* A light is basically a model can be used to iluminate a scene. */
	class Light : public Model {
	public:
		glm::vec3 ambient = glm::vec3(0.0);
		glm::vec3 diffuse = glm::vec3(0.0);
		glm::vec3 specular = glm::vec3(0.0);
		
		Light(glm::vec3 ambient = glm::vec3(0.0), glm::vec3 diffuse = glm::vec3(1.0), glm::vec3 specular = glm::vec3(1.0)) : Model() {
			this->ambient = ambient;
			this->diffuse = diffuse;
			this->specular = specular;
		}

		glm::vec3 getAmbientColor() {
			return ambient;
		}

		glm::vec3 getDiffuseColor() {
			return diffuse;
		}

		glm::vec3 getSpecularColor() {
			return specular;
		}
	};
}

#include "PointLight.hpp"