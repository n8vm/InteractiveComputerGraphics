#pragma once

#include "Mesh.hpp"

#include <glm/glm.hpp>
#include <array>

namespace Components::Meshes {
	class Cube : public MeshInterface {
	public:
		static std::shared_ptr<Mesh> Create(std::string name) {
			std::cout << "ECS::CM: Adding Cube \"" << name << "\"" << std::endl;

			auto meshComponent = std::make_shared<Mesh>();
			meshComponent->mesh = std::make_unique<Cube>();
			Systems::ComponentManager::Meshes[name] = meshComponent;
			return meshComponent;
		}

		void cleanup() {
			/* Destroy index buffer */
			vkDestroyBuffer(VKDK::device, indexBuffer, nullptr);
			vkFreeMemory(VKDK::device, indexBufferMemory, nullptr);

			/* Destroy vertex buffer */
			vkDestroyBuffer(VKDK::device, vertexBuffer, nullptr);
			vkFreeMemory(VKDK::device, vertexBufferMemory, nullptr);

			/* Destroy normal buffer */
			vkDestroyBuffer(VKDK::device, normalBuffer, nullptr);
			vkFreeMemory(VKDK::device, normalBufferMemory, nullptr);

			/* Destroy uv buffer */
			vkDestroyBuffer(VKDK::device, texCoordBuffer, nullptr);
			vkFreeMemory(VKDK::device, texCoordBufferMemory, nullptr);
		}

		Cube() {
			createVertexBuffer();
			createTexCoordBuffer();
			createIndexBuffer();
			createNormalBuffer();
		}
		
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

		uint32_t getTotalIndices() {
			return 36;
		}

		int getIndexBytes() {
			return sizeof(uint16_t);
		}
		
		glm::vec3 getCentroid() {
			return glm::vec3(0);
		}

	private:
		std::array<float, 72> verts = {
			1.0, -1.0, -1.0, 1.0, -1.0, -1.0,
			1.0, -1.0, -1.0, 1.0, -1.0, 1.0,
			1.0, -1.0, 1.0, 1.0, -1.0, 1.0,
			-1.0, -1.0, 1.0, -1.0, -1.0, 1.0,
			-1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
			-1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
			1.0, 1.0, -1.0, 1.0, 1.0, -1.0,
			1.0, 1.0, -1.0, 1.0, 1.0, 1.0,
			1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
			-1.0, 1.0, 1.0, -1.0, 1.0, 1.0,
			-1.0, 1.0, 1.0, -1.0, 1.0,-1.0,
			-1.0, 1.0,-1.0, -1.0, 1.0,-1.0
		};
		/* TODO: get real normals for this cube */
		std::array<float, 72> normals = {
			1.0, -1.0, -1.0, 1.0, -1.0, -1.0,
			1.0, -1.0, -1.0, 1.0, -1.0, 1.0,
			1.0, -1.0, 1.0, 1.0, -1.0, 1.0,
			-1.0, -1.0, 1.0, -1.0, -1.0, 1.0,
			-1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
			-1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
			1.0, 1.0, -1.0, 1.0, 1.0, -1.0,
			1.0, 1.0, -1.0, 1.0, 1.0, 1.0,
			1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
			-1.0, 1.0, 1.0, -1.0, 1.0, 1.0,
			-1.0, 1.0, 1.0, -1.0, 1.0,-1.0,
			-1.0, 1.0,-1.0, -1.0, 1.0,-1.0
		};
		std::array<float, 48> uvs = {
			0.000199999995f, 0.999800026f, 0.000199999995f, 0.666467011f,
			0.666467011f, 0.333133996f, 0.333133996f, 0.999800026f,
			0.333133996f, 0.666467011f, 0.333532989f, 0.666467011f,
			0.333133996f, 0.666867018f, 0.666467011f, 0.666467011f,
			0.999800026f, 0.000199999995f, 0.000199999995f, 0.666867018f,
			0.999800026f, 0.333133996f, 0.666467011f, 0.000199999995f,
			0.000199999995f, 0.333133996f, 0.000199999995f, 0.333532989f,
			0.333532989f, 0.333133996f, 0.333133996f, 0.333133996f,
			0.333133996f, 0.333532989f, 0.333532989f, 0.333532989f,
			0.333133996f, 0.000199999995f, 0.666467011f, 0.333532989f,
			0.666866004f, 0.000199999995f, 0.000199999995f, 0.000199999995f,
			0.666866004f, 0.333133012f, 0.333532989f, 0.000199999995f,
		};
		std::array<uint16_t, 36> indices = {
			0, 3, 6, 0, 6, 9, 12, 21, 18, 12, 18, 15, 1, 13,
			16, 1, 16, 4, 5, 17, 19, 5, 19, 7, 8, 20, 22, 8, 22,
			10, 14, 2, 11, 14, 11, 23,
		};

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		VkBuffer normalBuffer;
		VkDeviceMemory normalBufferMemory;

		VkBuffer texCoordBuffer;
		VkDeviceMemory texCoordBufferMemory;

		void createVertexBuffer() {
			VkDeviceSize bufferSize = verts.size() * sizeof(float);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our vertex data, then unmap */
			memcpy(data, verts.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createIndexBuffer() {
			VkDeviceSize bufferSize = indices.size() * sizeof(unsigned short);
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
			VkDeviceSize bufferSize = normals.size() * sizeof(float);
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
			VkDeviceSize bufferSize = uvs.size() * sizeof(float);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our normal data, then unmap */
			memcpy(data, uvs.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texCoordBuffer, texCoordBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, texCoordBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}
	};
}