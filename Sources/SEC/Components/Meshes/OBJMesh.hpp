#pragma once
#include "vkdk.hpp"
#include "Mesh.hpp"

namespace Components::Meshes {
	/* An obj mesh contains vertex information from an obj file that has been loaded to the GPU. */
	class OBJMesh : public Mesh {
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

		obj::Model model;
		/* Loads a mesh from an obj file */
		void loadFromOBJ(std::string objPath) {
			model = obj::loadModelFromFile(objPath);
			computeCentroid();

			/* TODO: Upload data to a vulkan device buffer */
			createVertexBuffer();
			createIndexBuffer();
			createNormalBuffer();
			createTexCoordBuffer();
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
			return model.faces.at("default").size();
		}

		void computeCentroid() {
			double x = 0.0, y = 0.0, z = 0.0;
			for (int i = 0; i < model.vertex.size(); i += 3) {
				x += model.vertex[i];
				y += model.vertex[i + 1];
				z += model.vertex[i + 2];
			}
			x /= (model.vertex.size() / 3);
			y /= (model.vertex.size() / 3);
			z /= (model.vertex.size() / 3);
			centroid = glm::vec3(x, y, z);
		}

		glm::vec3 getCentroid() {
			return centroid;
		}

	private:
		glm::vec3 centroid = glm::vec3(0.0);

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		VkBuffer normalBuffer;
		VkDeviceMemory normalBufferMemory;

		VkBuffer texCoordBuffer;
		VkDeviceMemory texCoordBufferMemory;

		void createVertexBuffer() {
			VkDeviceSize bufferSize = model.vertex.size() * sizeof(float);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our vertex data, then unmap */
			memcpy(data, model.vertex.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createIndexBuffer() {
			VkDeviceSize bufferSize = model.faces.at("default").size() * sizeof(unsigned short);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, model.faces.at("default").data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createNormalBuffer() {
			VkDeviceSize bufferSize = model.normal.size() * sizeof(float);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our normal data, then unmap */
			memcpy(data, model.normal.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normalBuffer, normalBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, normalBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

		void createTexCoordBuffer() {
			VkDeviceSize bufferSize = model.texCoord.size() * sizeof(float);
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			/* Map the memory to a pointer on the host */
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, bufferSize, 0, &data);

			/* Copy over our normal data, then unmap */
			memcpy(data, model.texCoord.data(), (size_t)bufferSize);
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texCoordBuffer, texCoordBufferMemory);
			VKDK::CopyBuffer(stagingBuffer, texCoordBuffer, bufferSize);

			/* Clean up the staging buffer */
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}

	};
}

