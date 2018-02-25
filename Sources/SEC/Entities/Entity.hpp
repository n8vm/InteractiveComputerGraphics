/* 
	Entities can be added to the scene.
	The scene renders any renderable entities.
*/

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "Components/Transform.hpp"
#include "vkdk.hpp"

class Scene;

namespace Entities {

	/* An entity is an object with a simple transform component, and a list of children. 
			Entities facilitate scene graph traversal, and can be extended to incorporate components like
				meshes, textures, sounds, etc. 
	*/
	class Entity {
	public:
		/* An Entity has a transform component, which moves the entity around */
		Components::Transform transform = Components::Transform();

		/* An Entity can have a list of child objects. By default, these objects are transformed 
		relative to this entity. */
		std::unordered_map<std::string, std::shared_ptr<Entity>> children;
		
		/* If an object isn't active, it and it's children aren't updated/rendered/raycast. */
		bool active = true;

		/* Entities by themselves don't render. This is for scene graph traversal */
		virtual void prerender(VkCommandBuffer &commandBuffer, VkRenderPass &renderPass, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			glm::mat4 new_model = model * transform.LocalToParentMatrix();
			
			for (auto i : children) {
				if (i.second.get()->active) {
					i.second->prerender(commandBuffer, renderPass, new_model, view, projection);
				}
			}
		};

		/* Entities by themselves don't render. This is for scene graph traversal */
		virtual void render(Scene *scene) {
			for (auto i : children) {
				if (i.second.get()->active) {
					i.second->render(scene);
				}
			}
		};

		void(*updateCallback) (Entity*) = nullptr;

		virtual void update(glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			if (updateCallback) updateCallback(this);

			glm::mat4 new_model = model * transform.LocalToParentMatrix();
			for (auto i : children) {
				if (i.second.get()->active) {
					i.second->update(new_model, view, projection);
				}
			}
		};
	
		virtual void cleanup() {
			for (auto i : children) {
				i.second->cleanup();
			}
		};


		/* Each entity receives a recursive point and direction, such that p and d */
		virtual void raycast(glm::vec4 point, glm::vec4 direction) {
			for (auto i : children) {
				if (i.second.get()->active) {
					/* Transform the ray */
					glm::vec4 newPoint = i.second->transform.ParentToLocalMatrix() * point;
					glm::vec4 newDirection = (i.second->transform.ParentToLocalMatrix() * (point + direction)) - newPoint;
					i.second->raycast(newPoint, newDirection);
				}
			}
		}
		
		void addObject(const std::string key, std::shared_ptr<Entity> object) {
			children[key] = object;
		}

		void removeObject(const std::string key) {
			children.erase(key);
		}
	};
}

/* Is there a better way to do this? trying to avoid infinite includes... */
//#include "Entities/Model/Model.hpp"
