#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/SpinTableCamera.hpp"
#include "Entities/Lights/Light.hpp"

#include "Components/Meshes/Mesh.hpp"

#include "Components/Textures/Texture2D.hpp"

using namespace std;

void System::RenderLoop() {
	using namespace VKDK;
	auto lastTime = glfwGetTime();

	/* Record the commands required to render the current scene. */
	System::MainScene->recordRenderPass();

	int oldWidth = VKDK::CurrentWindowSize[0], oldHeight = VKDK::CurrentWindowSize[1];
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();
		
		/* Update Camera Buffer */
		System::MainScene->updateCameraUBO();

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
		if (SubmitFrame() == true) System::MainScene->recordRenderPass();

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

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	PipelineParameters parameters = {};
	parameters.renderPass = MainScene->getRenderPass();

	/* Initialize Materials */
	Materials::SurfaceMaterials::UniformColoredSurface::Initialize(1, { parameters });
	Materials::SurfaceMaterials::TexturedBlinnSurface::Initialize(3, { parameters });

	/* Load a test texture */
	TextureList["ChickenTexture"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/ChickenTexture.ktx");
	TextureList["ChickenTexture_s"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/ChickenTexture.ktx");
	TextureList["GrassTexture"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/grass.ktx");
		
	/* Load the model (by default, a teapot) */
	MeshList["Body"] = make_shared<Meshes::OBJMesh>(ResourcePath "Chicken/Chicken.obj");
	MeshList["Eyes"] = make_shared<Meshes::OBJMesh>(ResourcePath "Chicken/Eyes.obj");
	MeshList["plane"] = make_shared<Meshes::Plane>();
	MeshList["sphere"] = make_shared<Meshes::Sphere>();
}

double lastx, lasty, x, y;
void UpdateLight(Entities::Entity *light) {
	glfwGetCursorPos(VKDK::DefaultWindow, &x, &y);
	if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) {
		light->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.up, x - lastx);
		light->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.right, y - lasty);
	}
	else {
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), .2f);
	}
	lastx = x;
	lasty = y;
}

void System::SetupEntities() {
	using namespace std;
	using namespace Entities;
	using namespace Components::Materials;

	/* Create a material for that mesh */
	glm::vec4 white = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec4 grey = glm::vec4(.1, .1, .1, 1.0);
	glm::vec4 red = glm::vec4(1.0, 0.0, 0.0, 1.0);
	glm::vec4 green = glm::vec4(0.0, 1.0, 0.0, 1.0);
	glm::vec4 blue = glm::vec4(0.0, 0.0, 1.0, 1.0);

	glm::vec4 la = glm::vec4(.3, .3, .3, 1.0);
	glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0);
	glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0);
	auto lightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(System::MainScene);
	lightSurface->setColor(la + ld + ls);

	/* Create an orbit camera to look at the model */
	glm::vec3 centriod = MeshList["Body"]->getCentroid();
	centriod *= .3;
	System::MainScene->camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(3.0, -5.0, 3.0), centriod, glm::vec3(0.0, 0.0, 1.0));
	System::MainScene->entities->addObject("camera", System::MainScene->camera);
	
	///* Floor */
	auto floorMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(System::MainScene, TextureList["GrassTexture"]);
	floorMaterial->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(.8, .8, .8, 1.0), glm::vec4(.0, .0, .0, 0.0));
	std::shared_ptr<Entities::Model> Floor = make_shared<Model>();
	Floor->transform.SetScale(5.0, 5.0, 5.0);
	Floor->setMesh(MeshList["plane"]);
	Floor->addMaterial(floorMaterial);
	System::MainScene->entities->addObject("Floor", Floor);

	/* Body */
	auto chickenMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(System::MainScene, TextureList["ChickenTexture"], TextureList["ChickenTexture_s"]);
	std::shared_ptr<Entities::Model> Body = make_shared<Model>();
	Body->transform.SetScale(.2, .2, .2);
	Body->setMesh(MeshList["Body"]);
	Body->addMaterial(chickenMaterial);
	System::MainScene->entities->addObject("Base", Body);

	/* Eyes */
	auto eyeMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(System::MainScene, TextureList["ChickenTexture"]);
	std::shared_ptr<Entities::Model> Eyes = make_shared<Model>();
	Eyes->transform.SetScale(.2, .2, .2);
	Eyes->setMesh(MeshList["Eyes"]);
	Eyes->addMaterial(eyeMaterial);
	System::MainScene->entities->addObject("Eyes", Eyes);

	/* Add a light to the scene */
	auto light = make_shared<Lights::Light>(la, ld, ls);
	light->transform.SetPosition(glm::vec3(0.0, -3.0, 3.0));
	light->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
	light->addMaterial(lightSurface);
	light->setMesh(MeshList["sphere"]);
	light->updateCallback = &UpdateLight;
	System::MainScene->entities->addObject("light", light);
	System::MainScene->LightList.emplace("light", light);
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
	for (auto &tex : TextureList) { tex.second->cleanup(); }
	
	Components::Materials::SurfaceMaterials::UniformColoredSurface::Destroy();
	Components::Materials::SurfaceMaterials::TexturedBlinnSurface::Destroy();
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
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 4 - Textures", false, false, true };
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