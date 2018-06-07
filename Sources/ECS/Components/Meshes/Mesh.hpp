#pragma once
#include "vkdk.hpp"
#include "Components/Component.hpp"
#include "Systems/ComponentManager.hpp"

namespace Components::Meshes {
	/* A mesh contains vertex information that has been loaded to the GPU. */
	class MeshInterface {
	public:
		class Vertex {
		public:
			glm::vec3 point = glm::vec3(0.0);
			glm::vec4 color = glm::vec4(1, 0, 1, 1);
			glm::vec3 normal = glm::vec3(0.0);
			glm::vec2 texcoord = glm::vec2(0.0);

			bool operator==(const Vertex &other) const
			{
				bool result =
					(point == other.point
						&& color == other.color
						&& normal == other.normal
						&& texcoord == other.texcoord);
				return result;
			}
		};

		virtual void cleanup() = 0;
		virtual int getIndexBytes() = 0;
		virtual VkBuffer getVertexBuffer() = 0;
		virtual VkBuffer getIndexBuffer() = 0;
		virtual VkBuffer getTexCoordBuffer() = 0;
		virtual VkBuffer getNormalBuffer() = 0;
		virtual uint32_t getTotalIndices() = 0;
		virtual glm::vec3 getCentroid() = 0;
	};

	/* A mesh component contains a mesh object */
	class Mesh : public Component {
	public:
		std::unique_ptr<Components::Meshes::MeshInterface> mesh;
		void cleanup() { 
			mesh->cleanup(); 
		}
	};
}
