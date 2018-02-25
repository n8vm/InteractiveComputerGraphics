#pragma once

#include "Mesh.hpp"

#include <glm/glm.hpp>
#include <array>

namespace Components::Meshes {
	class Plane : public Mesh {
	public:
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

		Plane() {
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
			return 6;
		}

		glm::vec3 getCentroid() {
			return glm::vec3(0);
		}

	private:
		std::array<float, 12> verts = {
			-1.f,
			-1.f,
			0.f,
			1.f,
			-1.f,
			0.f,
			-1.f,
			1.f,
			0.f,
			1.f,
			1.f,
			0.f
		};
		/* TODO: get real normals for this cube */
		std::array<float, 12> normals = {
			0.f,
			0.f,
			1.f,
			0.f,
			0.f,
			1.f,
			0.f,
			0.f,
			1.f,
			0.f,
			0.f,
			1.f
		};
		std::array<float, 8> uvs = {
			0.f,
			1.f,
			1.f,
			1.f,
			0.f,
			0.f,
			1.f,
			0.f
		};
		std::array<unsigned short, 36> indices = {
			0,
			1,
			3,
			0,
			3,
			2
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