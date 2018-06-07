#pragma once
#include "Entities/Entity.hpp"
#include "Components/Materials/Material.hpp"

class Scene;

namespace Entities {
	
	class Model : public Entity {
	public:
		Model(std::string name) : Entity(name) {
		}

		/* A model has a list of material components, which are used to render it */
		std::vector < std::shared_ptr<Components::Materials::MaterialInterface> > materials;

		/* A model has a mesh component, which is used by the materials for rendering */
		std::shared_ptr<Components::Meshes::MeshInterface> mesh;
		
		/* Models use their materials to render meshes */
		virtual void render(VkCommandBuffer commandBuffer, VkRenderPass renderpass) {
			for (int i = 0; i < materials.size(); ++i) {
				materials[i]->render(commandBuffer, renderpass, mesh);
			}

			Entity::render(commandBuffer, renderpass);
		};

		virtual void uploadUniforms(glm::mat4 model) {
			glm::mat4 new_model = model * transform.LocalToParentMatrix();
			
			/* Update uniform buffer */
			TransformBufferObject tbo = {};
			tbo.modelMatrix = new_model;

			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, transformUBOMemory, 0, sizeof(tbo), 0, &data);
			memcpy(data, &tbo, sizeof(tbo));
			vkUnmapMemory(VKDK::device, transformUBOMemory);

			/* Allow any materials attached to this model to upload their uniform values */
			for (int i = 0; i < materials.size(); ++i) {
				materials[i]->uploadUniforms();
			}

			Entity::uploadUniforms(model);
		}

		void cleanup() {
			/*for (auto &mat : materials) {
				mat->cleanup();
			}*/
		}

		void setMesh(std::shared_ptr<Components::Meshes::MeshInterface> mesh) {
			this->mesh = mesh;
		}

		//void addMaterial(std::shared_ptr<Components::Materials::Material> material, Components::Math::Perspective &perspective) {
		//	/* Create a descriptor set for this material */
		//	material->createDescriporSet(perspective.getUBO(), transformUBO);
		//	materials.push_back(material);
		//}


	};
}