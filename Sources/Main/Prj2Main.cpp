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
			Components::Materials::HaloMaterials::UniformColoredPoints::RefreshPipeline();
			continue;
		};


		/* Get current image index */
		currentImageIndex = VKDK::GetNextImageIndex();

		/* Go through the scene, recording render passes into the current command buffer */
		VKDK::StartCommandBufferRecording(currentImageIndex);

		// get camera working 
		System::World.render(glm::mat4(1.0), camera->getView(), camera->getProjection());

		/* Stop recording the command buffer */
		VKDK::StopCommandBufferRecording(currentImageIndex);

		/* Submit command buffer to graphics queue for rendering */
		VKDK::DrawFrame(currentImageIndex);
	}

	vkDeviceWaitIdle(VKDK::device);
}

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	/* Initialize Materials */
	Materials::HaloMaterials::UniformColoredPoints::Initialize(3);
	
	/* Load the model (by default, a teapot) */
	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->loadFromOBJ(Options::objLocation);
	MeshList.emplace("mesh", mesh);
}

void UpdateTeapot1(Entities::Entity *teapot) {
	teapot->transform.AddRotation(glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)));
}

void UpdateTeapot2(Entities::Entity *teapot) {
	teapot->transform.AddRotation(glm::angleAxis(-0.015f, glm::vec3(0.0, 0.0, 1.0)));
}

void UpdateTeapot3(Entities::Entity *teapot) {
	teapot->transform.AddRotation(glm::angleAxis(-0.005f, glm::vec3(0.0, 0.0, 1.0)));
}

void System::SetupEntities() {
	using namespace std;
	using namespace Entities;
	using namespace Components::Materials;

	/* Create a material for that mesh */
	auto redPoints = make_shared<HaloMaterials::UniformColoredPoints>();
	redPoints->setColor(1.0, 0.0, 0.0, 1.0);

	auto greenPoints = make_shared<HaloMaterials::UniformColoredPoints>();
	greenPoints->setColor(0.0, 1.0, 0.0, 1.0);

	auto bluePoints = make_shared<HaloMaterials::UniformColoredPoints>();
	bluePoints->setColor(0.0, 0.0, 1.0, 1.0);	

	/* Create an entity with the provided mesh and model */
	std::shared_ptr<Entities::Model> redTeapot = make_shared<Model>();
	redTeapot->transform.SetPosition(-1.5, 0.0, 0.0);
	redTeapot->transform.SetScale(.1, .1, .1);
	redTeapot->setMesh(MeshList["mesh"]);
	redTeapot->addMaterial(redPoints);
	redTeapot->updateCallback = &UpdateTeapot1;
	World.addObject("redTeapot", redTeapot);

	std::shared_ptr<Entities::Model> greenTeapot = make_shared<Model>();
	greenTeapot->transform.SetPosition(0.0, -1.5, 0.0);
	greenTeapot->transform.SetScale(.1, .1, .1);
	greenTeapot->setMesh(MeshList["mesh"]);
	greenTeapot->addMaterial(greenPoints);
	greenTeapot->updateCallback = &UpdateTeapot2;
	World.addObject("greenTeapot", greenTeapot);

	std::shared_ptr<Entities::Model> blueTeapot = make_shared<Model>();
	blueTeapot->transform.SetPosition(1.5, 0.0, 0.0);
	blueTeapot->transform.SetScale(.1, .1, .1);
	blueTeapot->setMesh(MeshList["mesh"]);
	blueTeapot->addMaterial(bluePoints);
	blueTeapot->updateCallback = &UpdateTeapot3;
	World.addObject("blueTeapot", blueTeapot);

	glm::vec3 centriod = MeshList["mesh"]->getCentroid();
	centriod *= .1;

	/* Create an orbit camera to look at the model */
	auto camera = make_shared<Cameras::OrbitCamera>(glm::vec3(0.0, -5.0, 5.0), centriod);
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
	
	for (auto &mesh : MeshList) { mesh.second->cleanup(); }
	System::World.cleanup();

	Components::Materials::HaloMaterials::UniformColoredPoints::Destroy();
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