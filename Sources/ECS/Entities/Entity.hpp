// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │  Entity: Most basic object in a scene.                           |
// |    An Entity can own a set of components, and by default has a   |
// |    transform component. Entities can have zero to many children, |
// |    and a reference to a parent entity.                           |
// └──────────────────────────────────────────────────────────────────┘

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <typeindex>

#include "Systems/ComponentManager.hpp"
#include "Systems/SceneGraph.hpp"
#include "Components/Math/Transform.hpp"
#include "Components/Math/Perspective.hpp"
#include "Components/Callbacks/Callbacks.hpp"
#include "vkdk.hpp"

namespace Entities {

	/* An entity is an object with a simple transform component, and a list of children. 
			Entities facilitate scene graph traversal, and can be extended to incorporate components like
				meshes, textures, sounds, etc. 
	*/
	class Entity : public std::enable_shared_from_this<Entity> {
	public:
		/* Used as a key within it's parent's children */
		std::string name;

		/* If an entity isn't active, its callbacks arent called */
		bool active = true;

		std::shared_ptr<Entity> parent = nullptr;

		std::unordered_map<std::string, std::shared_ptr<Entity>> children;

		std::shared_ptr<Components::Math::Transform> transform;

		std::shared_ptr<Components::Callbacks> callbacks;

		std::map<std::type_index, std::vector<std::shared_ptr<Component>>> components;

		/* To add an additional component, use this */
		template <typename T>
		void addComponent(T component) {
			components[std::type_index(typeid(*component))].push_back(component);
		}

		/* Variadic template for adding multiple components simultaneously */
		template <typename T, typename ... Args>
		void addComponent(T component, Args ... args) {
			components[std::type_index(typeid(*component))].push_back(component);
			addComponent(args...);
		}

		/* To retrieve an additional component, use this */
		template <typename T>
		std::shared_ptr<T> getFirstComponent()
		{
			std::type_index index(typeid(T));
			if (components.count(std::type_index(typeid(T))) != 0)
			{
				return std::static_pointer_cast<T>(components[index][0]);
			}
			else
			{
				return nullptr;
			}
		}

		/* To retrieve an additional component, use this */
		template <typename T>
		std::vector<std::shared_ptr<T>> getComponents()
		{
			std::vector<std::shared_ptr<T>> result;
			std::type_index index(typeid(T));
			if (components.count(std::type_index(typeid(T))) != 0)
			{
				std::vector < std::shared_ptr<Component>> componentSet = components[index];
				for (int i = 0; i < componentSet.size(); ++i) {
					result.push_back(std::static_pointer_cast<T>(componentSet[i]));
				}
			}
			return result;
		}

		static std::shared_ptr<Entity> Create(std::string name) {
			auto entity = std::make_shared<Entity>(name);
			Systems::SceneGraph::Entities[name] = entity;
			return entity;
		}

		Entity(std::string name) : enable_shared_from_this() {
			this->name = name;
			transform = Components::Math::Transform::Create(name);
			callbacks = Components::Callbacks::Create(name);
		}

		void setParent(std::shared_ptr<Entity> parent) {
			this->parent = parent;
			parent->children[name] = shared_from_this();
		}

		void addChild(std::shared_ptr<Entity> object) {
			children[object->name] = object;
			children[object->name]->parent = shared_from_this();
		}

		void removeChild(std::shared_ptr<Entity> object) {
			children.erase(object->name);
		}

		glm::mat4 getWorldToLocalMatrix() {
			glm::mat4 parentMatrix = glm::mat4(1.0);
			if (parent != nullptr) {
				parentMatrix = parent->getWorldToLocalMatrix();
				return transform->ParentToLocalMatrix() * parentMatrix;
			}
			else return transform->ParentToLocalMatrix();
		}

		glm::mat4 getLocalToWorldMatrix() {
			return glm::inverse(getWorldToLocalMatrix());
		}
	};
}



		

		///* An Entity has one to many perspectives, which allows the entity to "see" into the scene */
		//std::vector<Components::Math::Perspective> perspectives = {};





		//


		
		//virtual void prerender(VkCommandBuffer &commandBuffer, VkRenderPass &renderPass, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
		//	glm::mat4 new_model = model * transform.LocalToParentMatrix();
		//	
		//	for (auto i : children) {
		//		if (i.second.get()->active) {
		//			i.second->prerender(commandBuffer, renderPass, new_model, view, projection);
		//		}
		//	}
		//};

		//virtual void render(VkCommandBuffer commandBuffer, VkRenderPass renderpass) {
		//	for (auto i : children) {
		//		if (i.second.get()->active) {
		//			i.second->render(commandBuffer, renderpass);
		//		}
		//	}
		//};

		//virtual void update() {
		//	if (updateCallback) updateCallback(shared_from_this());
		//	for (auto i : children) {
		//		if (i.second.get()->active) {
		//			i.second->update();
		//		}
		//	}
		//}

		//virtual void uploadUniforms(glm::mat4 model = glm::mat4(1.0)) {
		//	glm::mat4 new_model = model * transform.LocalToParentMatrix();
		//	for (auto i : children) {
		//		if (i.second.get()->active) {
		//			i.second->uploadUniforms(new_model);
		//		}
		//	}
		//};
		//
		//virtual void raycast(glm::vec4 point, glm::vec4 direction) {
		//	for (auto i : children) {
		//		if (i.second.get()->active) {
		//			/* Transform the ray */
		//			glm::vec4 newPoint = i.second->transform.ParentToLocalMatrix() * point;
		//			glm::vec4 newDirection = (i.second->transform.ParentToLocalMatrix() * (point + direction)) - newPoint;
		//			i.second->raycast(newPoint, newDirection);
		//		}
		//	}
		//}

		//virtual void cleanup() {
		//	for (auto i : perspectives) {
		//		i.cleanup();
		//	}

		//	for (auto i : children) {
		//		i.second->cleanup();
		//	}
		//};

		///* Records draw calls from the perspective of this entities perspectives */
		//virtual void recordRenderPass(std::shared_ptr<Entities::Entity> scene, glm::vec4 clearColor = glm::vec4(0.0), float clearDepth = 1.0f, uint32_t clearStencil = 0) {
		//	for (int i = 0; i < perspectives.size(); ++i) {
		//		perspectives[i].recordRenderPass(shared_from_this(), clearColor, clearDepth, clearStencil);
		//	}
		//}




