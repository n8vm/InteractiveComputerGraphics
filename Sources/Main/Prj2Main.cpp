#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/OrbitCamera.hpp"

using namespace std;


void System::UpdateLoop() {
	auto lastTime = glfwGetTime();
	while (quit == false) {
		auto currentTime = glfwGetTime();
		if (currentTime - lastTime > 1.0 / UpdateRate) {
			World.update();
			lastTime = currentTime;
		}
	}
}

void System::RenderLoop() {
	shared_ptr<Entities::Cameras::OrbitCamera> camera =
		dynamic_pointer_cast<Entities::Cameras::OrbitCamera>(World.children.at("camera"));

	int oldWidth = VKDK::DefaultWindowSize[0], oldHeight = VKDK::DefaultWindowSize[1];
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		glfwPollEvents();
		
		/* If the window resized, then the swapchain updated, so we need to update the material pipelines */
		int width, height;
		glfwGetWindowSize(VKDK::DefaultWindow, &width, &height);
		if (oldWidth != width || oldHeight != height) {
			oldWidth = width; oldHeight = height;
			camera->setWindowSize(width, height);
			for (auto &mat : System::MaterialList) mat.second->refresh();
			continue;
		};


		/* Get current image index */
		currentImageIndex = VKDK::GetNextImageIndex();

		/* Go through the scene, recording render passes into the current command buffer */
		VKDK::StartCommandBufferRecording(currentImageIndex);

		// get camera working 
		System::World.render(camera->getModel(), camera->getView(), camera->getProjection());

		/* Stop recording the command buffer */
		VKDK::StopCommandBufferRecording(currentImageIndex);

		/* Submit command buffer to graphics queue for rendering */
		VKDK::DrawFrame(currentImageIndex);
	}

	vkDeviceWaitIdle(VKDK::device);
}

void System::SetupComponents() {
	using namespace Components;
	using namespace std;

	/* Load the model (by default, a teapot) */
	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->loadFromOBJ(Options::objLocation);
	MeshList.emplace("mesh", mesh);

	/* Create a material for that mesh */
	shared_ptr<Materials::HaloMaterials::UniformColoredPoints> pointMaterial
		= make_shared<Materials::HaloMaterials::UniformColoredPoints>();
	pointMaterial->setColor(1.0, 0.0, 0.0, 1.0);
	MaterialList.emplace("pointMaterial", pointMaterial);
}

void System::SetupEntities() {
	using namespace Entities;

	/* Create an entity with the provided mesh and model */
	std:shared_ptr<Entities::Model> model = make_shared<Model>();
	model->setMesh(MeshList["mesh"]);
	model->addMaterial(MaterialList["pointMaterial"]);
	World.addObject("model", model);

	glm::vec3 centriod = MeshList["mesh"]->getCentroid();

	/* Create an orbit camera to look at the model */
	std::shared_ptr<Entities::Cameras::OrbitCamera> camera = 
		make_shared<Entities::Cameras::OrbitCamera>(glm::vec3(0.0, 0.0, 40.0), centriod);
	World.addObject("camera", camera);
}

void System::Start() {
	if (System::quit) return;
	
	/* Update on seperate thread */
	System::UpdateThread = new std::thread(System::UpdateLoop);

	/* Render on the current thread */
	System::RenderLoop();
}

void System::Terminate() {
	/* Quit */
	System::quit = true;

	System::UpdateThread->join();
	
	delete(System::UpdateThread);

	for (auto &mesh : MeshList) {
		mesh.second->cleanup();
	}

	for (auto &material : MaterialList) {
		material.second->cleanup();
	}
}

int main(int argc, char** argv) {
	/* Process arguments */
	Options::ProcessArgs(argc, argv);

	/* Initialize Vulkan */
	VKDK::InitializationParameters vkdkParams = { 512, 512, "Project 2 - Transformations", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) System::quit = true;

	/* Setup System Entity Component */
	System::SetupComponents();
	System::SetupEntities();
	System::Start();

	/* Cleanup */
	System::Terminate();
	VKDK::Terminate();	
}