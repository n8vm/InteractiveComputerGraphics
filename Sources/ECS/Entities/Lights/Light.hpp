#pragma once
#include "Entities/Entity.hpp"
#include "Entities/Model/Model.hpp"

namespace Entities::Lights {
	/* A light is basically a model can be used to iluminate a scene. */
	class Light : public Model {
	public:
		glm::vec3 ambient = glm::vec3(0.0);
		glm::vec3 diffuse = glm::vec3(0.0);
		glm::vec3 specular = glm::vec3(0.0);
		
		glm::mat4 model = glm::mat4(1.0);
		glm::mat4 view = glm::mat4(1.0);
		glm::mat4 proj = glm::mat4(1.0);

		Light(glm::vec3 ambient = glm::vec3(0.0), glm::vec3 diffuse = glm::vec3(1.0), glm::vec3 specular = glm::vec3(1.0)) : Model() {
			this->ambient = ambient;
			this->diffuse = diffuse;
			this->specular = specular;
		}

		//void update(glm::mat4 model, glm::mat4 view, glm::mat4 proj) {
		//	this->model = model;
		//	Entity::update(model, view, proj);
		//}

		glm::vec3 getAmbientColor() {
			return ambient;
		}

		glm::vec3 getDiffuseColor() {
			return diffuse;
		}

		glm::vec3 getSpecularColor() {
			return specular;
		}
	
		//void setView(glm::mat4 view) {
		//	this->view = view;
		//}

		//void setProj(glm::mat4 proj) {
		//	this->proj = proj;
		//}

		//glm::mat4 getModel() {
		//	return model;
		//}

		//glm::mat4 getView() {
		//	return view;
		//}

		//glm::mat4 getProj() {
		//	return proj;
		//}
	};
}

#include "PointLight.hpp"