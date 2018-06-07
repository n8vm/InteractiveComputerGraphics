#pragma once
#include "vkdk.hpp"
#include "Mesh.hpp"

#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>

#include "tinyobjloader.h"

namespace Components::Meshes {
	/* An obj mesh contains vertex information from an obj file that has been loaded to the GPU. */
	class OBJMesh : public MeshInterface {
	public:
		static std::shared_ptr<Mesh> Create(std::string name, std::string filepath) {
			std::cout << "ComponentManager: Adding OBJMesh \"" << name << "\"" << std::endl;

			auto meshComponent = std::make_shared<Mesh>();
			meshComponent->mesh = std::make_unique<OBJMesh>(filepath);
			Systems::ComponentManager::Meshes[name] = meshComponent;
			return meshComponent;
		}
		OBJMesh(std::string filepath) {
			loadFromOBJ(filepath);
		}

	public:
		void cleanup() {
			/* Destroy index buffer */
			vkDestroyBuffer(VKDK::device, indexBuffer, nullptr);
			vkFreeMemory(VKDK::device, indexBufferMemory, nullptr);

			/* Destroy vertex buffer */
			vkDestroyBuffer(VKDK::device, vertexBuffer, nullptr);
			vkFreeMemory(VKDK::device, vertexBufferMemory, nullptr);

			/* Destroy vertex color buffer */
			vkDestroyBuffer(VKDK::device, colorBuffer, nullptr);
			vkFreeMemory(VKDK::device, colorBufferMemory, nullptr);

			/* Destroy normal buffer */
			vkDestroyBuffer(VKDK::device, normalBuffer, nullptr);
			vkFreeMemory(VKDK::device, normalBufferMemory, nullptr);

			/* Destroy uv buffer */
			vkDestroyBuffer(VKDK::device, texCoordBuffer, nullptr);
			vkFreeMemory(VKDK::device, texCoordBufferMemory, nullptr);
		}

		/* Loads a mesh from an obj file */
		void loadFromOBJ(std::string objPath);

		VkBuffer getVertexBuffer() {
			return vertexBuffer;
		}

		VkBuffer getIndexBuffer() {
			return indexBuffer;
		}

		VkBuffer getNormalBuffer() {
			return normalBuffer;
		}

		VkBuffer getTexCoordBuffer() {
			return texCoordBuffer;
		}

		int getIndexBytes() {
			return sizeof(uint32_t);
		}

		uint32_t getTotalIndices() {
			return (uint32_t)indices.size();
		}

		void computeCentroid() {
			glm::vec3 s(0.0);
			for (int i = 0; i < points.size(); i += 1) {
				s += points[i];
			}
			s /= points.size();
			centroid = s;
		}

		glm::vec3 getCentroid() {
			return centroid;
		}

		std::vector<glm::vec3> points;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec4> colors;
		std::vector<glm::vec2> texcoords;
		std::vector<uint32_t> indices;

		tinyobj::attrib_t attrib;

	private:
		glm::vec3 centroid = glm::vec3(0.0);

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		VkBuffer colorBuffer;
		VkDeviceMemory colorBufferMemory;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		VkBuffer normalBuffer;
		VkDeviceMemory normalBufferMemory;

		VkBuffer texCoordBuffer;
		VkDeviceMemory texCoordBufferMemory;

		void createVertexBuffer() {
			VkDeviceSize bufferSize = points.size() * sizeof(glm::vec3);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our vertex data, then unmap */
			memcpy(data, points.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createColorBuffer() {
			VkDeviceSize bufferSize = colors.size() * sizeof(glm::vec4);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our vertex data, then unmap */
			memcpy(data, colors.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorBuffer, colorBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, colorBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createIndexBuffer() {
			VkDeviceSize bufferSize = indices.size() * sizeof(uint32_t);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, indices.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createNormalBuffer() {
			VkDeviceSize bufferSize = normals.size() * sizeof(glm::vec3);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our normal data, then unmap */
			memcpy(data, normals.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normalBuffer, normalBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, normalBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createTexCoordBuffer() {
			VkDeviceSize bufferSize = texcoords.size() * sizeof(glm::vec2);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our normal data, then unmap */
			memcpy(data, texcoords.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texCoordBuffer, texCoordBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, texCoordBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}
	};
}

