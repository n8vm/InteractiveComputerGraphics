#pragma once
#include "vkdk.hpp"

namespace Components::Meshes {
	/* A mesh component contains vertex information that has been loaded to the GPU. */
	class Mesh {
	public:
		virtual void cleanup() = 0;
		virtual VkBuffer getVertexBuffer() = 0;
		virtual VkBuffer getIndexBuffer() = 0;
		virtual VkBuffer getTexCoordBuffer() = 0;
		virtual VkBuffer getNormalBuffer() = 0;
		virtual uint32_t getTotalIndices() = 0;
		virtual glm::vec3 getCentroid() = 0;
	};
}

#include "Components/Meshes/OBJMesh.hpp"
#include "Components/Meshes/Cube.hpp"
#include "Components/Meshes/Sphere.hpp"
#include "Components/Meshes/Plane.hpp"