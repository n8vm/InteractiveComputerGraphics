#pragma once
#include "Entities/Entity.hpp"
#include "Components/Materials/Material.hpp"

namespace Entities {
	
	class Model : public Entity {
	public:
		/* A model has a list of material components, which are used to render it */
		std::vector < std::shared_ptr<Components::Materials::Material> > materials;

		/* A model has a mesh component, which is used by the materials for rendering */
		std::shared_ptr<Components::Mesh> mesh;

		/* Models use their materials to render meshes */
		virtual void render(glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			glm::mat4 new_model = model * transform.LocalToParentMatrix();
			for (int i = 0; i < materials.size(); ++i) {
				materials[i]->render(mesh, new_model, view, projection);
			}

			Entity::render(model, view, projection);
		};

		void update() {
			/* Temporary */
			this->transform.AddRotation(glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)));
		}

		void setMesh(std::shared_ptr<Components::Mesh> mesh) {
			this->mesh = mesh;
		}

		void addMaterial(std::shared_ptr<Components::Materials::Material> material) {
			materials.push_back(material);
		}
	};
}