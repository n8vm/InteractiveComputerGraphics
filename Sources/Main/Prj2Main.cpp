#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/SpinTableCamera.hpp"

using namespace std;

void System::RenderLoop() {
	using namespace VKDK;
	auto lastTime = glfwGetTime();

	/* Record the commands required to render the current scene. */
	System::MainScene->recordRenderPass();

	int oldWidth = VKDK::CurrentWindowSize[0], oldHeight = VKDK::CurrentWindowSize[1];
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();
		/* Aquire a new image from the swapchain */
		if (PrepareFrame() == true) System::MainScene->recordRenderPass();

		/* Submit to graphics queue  */
		Scene::SubmitToGraphicsQueueInfo info;
		info.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
		info.graphicsQueue = VKDK::graphicsQueue;
		info.signalSemaphores = { VKDK::semaphores.renderComplete };
		info.waitSemaphores = { VKDK::semaphores.presentComplete };
		System::MainScene->submitToGraphicsQueue(info);
		
		/* Submit the frame for presenting. */
		if(SubmitFrame() == true) System::MainScene->recordRenderPass();
		
		std::cout << "\r Framerate: " << currentTime - lastTime;
		lastTime = currentTime;
	}
	vkDeviceWaitIdle(device);
}

void System::EventLoop() {
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		glfwPollEvents();
	}
}

void System::UpdateLoop() {
	auto lastTime = glfwGetTime();
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();
		if (currentTime - lastTime > 1.0 / UpdateRate) {
			System::MainScene->update();
			lastTime = currentTime;
		}
	}
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

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	/* Initialize Materials */
	Materials::HaloMaterials::UniformColoredPoint::Initialize(3, {MainScene->getRenderPass()});
	
	/* Load the model (by default, a teapot) */
	shared_ptr<Meshes::OBJMesh> mesh = make_shared<Meshes::OBJMesh>(Options::objLocation);
	MeshList.emplace("mesh", mesh);
}

void System::SetupEntities() {
	using namespace std;
	using namespace Entities;
	using namespace Components::Materials;

	/* Create a material for that mesh */
	auto redPoints = make_shared<HaloMaterials::UniformColoredPoint>(System::MainScene);
	redPoints->setColor(1.0, 0.0, 0.0, 1.0);
	redPoints->setPointSize(1.0);

	auto greenPoints = make_shared<HaloMaterials::UniformColoredPoint>(System::MainScene);
	greenPoints->setColor(0.0, 1.0, 0.0, 1.0);
	greenPoints->setPointSize(1.0);
	
	auto bluePoints = make_shared<HaloMaterials::UniformColoredPoint>(System::MainScene);
	bluePoints->setColor(0.0, 0.0, 1.0, 1.0);	
	bluePoints->setPointSize(1.0);

	/* Create an entity with the provided mesh and model */
	std::shared_ptr<Entities::Model> redTeapot = make_shared<Model>();
	redTeapot->transform.SetPosition(-1.5, 0.0, 0.0);
	redTeapot->transform.SetScale(.1, .1, .1);
	redTeapot->setMesh(MeshList["mesh"]);
	redTeapot->addMaterial(redPoints);
	redTeapot->updateCallback = &UpdateTeapot1;
	MainScene->entities->addObject("redTeapot", redTeapot);

	std::shared_ptr<Entities::Model> greenTeapot = make_shared<Model>();
	greenTeapot->transform.SetPosition(0.0, -1.5, 0.0);
	greenTeapot->transform.SetScale(.1, .1, .1);
	greenTeapot->setMesh(MeshList["mesh"]);
	greenTeapot->addMaterial(greenPoints);
	greenTeapot->updateCallback = &UpdateTeapot2;
	MainScene->entities->addObject("greenTeapot", greenTeapot);

	std::shared_ptr<Entities::Model> blueTeapot = make_shared<Model>();
	blueTeapot->transform.SetPosition(1.5, 0.0, 0.0);
	blueTeapot->transform.SetScale(.1, .1, .1);
	blueTeapot->setMesh(MeshList["mesh"]);
	blueTeapot->addMaterial(bluePoints);
	blueTeapot->updateCallback = &UpdateTeapot3;
	MainScene->entities->addObject("blueTeapot", blueTeapot);

	glm::vec3 centriod = MeshList["mesh"]->getCentroid();
	centriod *= .1;

	/* Create an orbit camera to look at the model */
	MainScene->camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(0.0, -5.0, 5.0), centriod, glm::vec3(0.0, 0.0, 1.0));
	MainScene->entities->addObject("camera", MainScene->camera);
}

void System::Start() {
	if (System::quit) return;
	
	/* Render on seperate thread */
	System::RenderThread = new std::thread(System::RenderLoop);

	/* Render on seperate thread */
	System::UpdateThread = new std::thread(System::UpdateLoop);

	/* Update events on the current thread */
	System::EventLoop();
}

void System::Cleanup() {
	/* Quit */
	System::quit = true;

	System::RenderThread->join();
	System::UpdateThread->join();

	delete(System::RenderThread);
	delete(System::UpdateThread);
	
	for (auto &mesh : MeshList) { mesh.second->cleanup(); }

	Components::Materials::HaloMaterials::UniformColoredPoint::Destroy();
}

void System::Initialize() {
	MainScene = make_shared<Scene>(true);
}

void System::Terminate() {
	MainScene->cleanup();
}

int main(int argc, char** argv) {
	/* Process arguments */
	Options::ProcessArgs(argc, argv);

	/* Initialize Vulkan */
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 2 - Transformations", false, false, true};
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) System::quit = true;

	System::Initialize();

	/* Setup System Entity Component */
	System::SetupComponents();
	System::SetupEntities();
	System::Start();
	System::Cleanup();

	/* Terminate */
	System::Terminate();
	VKDK::Terminate();	
}