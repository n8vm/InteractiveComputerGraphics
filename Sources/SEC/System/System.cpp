#include "System.hpp"

namespace System {
	using namespace std;
	atomic<bool> quit = false;

	thread *UpdateThread;
	thread *RaycastThread;
	int UpdateRate = 90;
	int FrameRate = 90;

	int currentImageIndex = 0;

	Entities::Entity World = Entities::Entity();
	//Entity Screen = Entity();

	std::shared_ptr<Entities::Cameras::Camera> camera;


	//unordered_map<string, std::shared_ptr<Texture>> TextureList;
	//unordered_map<string, shared_ptr<Components::Materials::Material>> MaterialList;
	unordered_map<string, shared_ptr<Components::Meshes::Mesh>> MeshList;
	unordered_map<std::string, std::shared_ptr<Entities::Lights::Light>> LightList;

	

	VkBuffer lightBuffer;
	VkDeviceMemory lightBufferMemory;

	VkBuffer cameraBuffer;
	VkDeviceMemory cameraBufferMemory;

	void createLightUniformBuffer() {
		VkDeviceSize bufferSize = sizeof(Entities::Lights::LightBufferObject);
		VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightBuffer, lightBufferMemory);
	}

	void createCameraUniformBuffer() {
		VkDeviceSize bufferSize = sizeof(Entities::Cameras::CameraBufferObject);
		VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cameraBuffer, cameraBufferMemory);
	}

	void Initialize() {
		createLightUniformBuffer();
		createCameraUniformBuffer();
	}

	void Terminate() {
		vkDestroyBuffer(VKDK::device, lightBuffer, nullptr);
		vkFreeMemory(VKDK::device, lightBufferMemory, nullptr);

		vkDestroyBuffer(VKDK::device, cameraBuffer, nullptr);
		vkFreeMemory(VKDK::device, cameraBufferMemory, nullptr);
	}
	
	void UpdateLightBuffer() {
		Entities::Lights::LightBufferObject lbo = {};

		/* To do: fill light buffer object here */
		lbo.numLights = LightList.size();
		int i = 0;
		for (auto light : LightList) {
			lbo.lights[i].position = glm::vec4(light.second->transform.GetPosition(), 1.0);
			lbo.lights[i].ambient = glm::vec4(light.second->getAmbientColor(), 1.0);
			lbo.lights[i].diffuse = glm::vec4(light.second->getDiffuseColor(), 1.0);
			lbo.lights[i].specular = glm::vec4(light.second->getSpecularColor(), 1.0);
			++i;
		}

		/* Map uniform buffer, copy data directly, then unmap */
		void* data;
		vkMapMemory(VKDK::device, lightBufferMemory, 0, sizeof(lbo), 0, &data);
		memcpy(data, &lbo, sizeof(lbo));
		vkUnmapMemory(VKDK::device, lightBufferMemory);
	}

	void UpdateCameraBuffer() {
		/* Update uniform buffer */
		Entities::Cameras::CameraBufferObject cbo = {};
		/* To do: fill camera buffer object here */
		cbo.View = camera->getView();
		cbo.Projection = camera->getProjection();
		cbo.Projection[1][1] *= -1; // required so that image doesn't flip upside down.
		
		/* Map uniform buffer, copy data directly, then unmap */
		void* data;
		vkMapMemory(VKDK::device, cameraBufferMemory, 0, sizeof(cbo), 0, &data);
		memcpy(data, &cbo, sizeof(cbo));
		vkUnmapMemory(VKDK::device, cameraBufferMemory);
	}

	VkBuffer GetLightBuffer() {
		return lightBuffer;
	}

	VkBuffer GetCameraBuffer() {
		return cameraBuffer;
	}

	//std::shared_ptr<Entity> Raycaster;
	//bool MouseDown[5]; // TODO: fix this kludge
};
