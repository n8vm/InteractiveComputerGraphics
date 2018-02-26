#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/SpinTableCamera.hpp"
#include "Entities/Lights/Light.hpp"

#include "Components/Meshes/Mesh.hpp"
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

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	/* Initialize Materials */
	PipelineParameters parameters = {};
	parameters.renderPass = MainScene->getRenderPass();

	Materials::SurfaceMaterials::UniformColoredSurface::Initialize(1, {parameters});
	Materials::SurfaceMaterials::BlinnSurface::Initialize(4, { parameters });
	
	/* Load the model (by default, a teapot) */
	auto mesh = make_shared<Meshes::OBJMesh>(Options::objLocation);
	MeshList.emplace("mesh", mesh);

	auto sphere = make_shared<Meshes::Sphere>();
	MeshList.emplace("sphere", sphere);

	auto plane = make_shared<Meshes::Plane>();
	MeshList.emplace("plane", plane);
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

	auto redSurface = make_shared<SurfaceMaterials::BlinnSurface>(System::MainScene);
	auto greenSurface = make_shared<SurfaceMaterials::BlinnSurface>(System::MainScene);
	auto blueSurface = make_shared<SurfaceMaterials::BlinnSurface>(System::MainScene);
	auto planeSurface = make_shared<SurfaceMaterials::BlinnSurface>(System::MainScene);

	redSurface->setColor(red, red, white);
	greenSurface->setColor(green, green, white);
	blueSurface->setColor(blue, blue, white);
	planeSurface->setColor(grey, grey, white);
	
	glm::vec4 la = glm::vec4(.1, .1, .1, 1.0);
	glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0);
	glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0);
	auto lightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(System::MainScene);
	lightSurface->setColor(la + ld + ls);

	/* Create an orbit camera to look at the model */
	glm::vec3 centriod = MeshList["mesh"]->getCentroid();
	centriod *= .1;
	System::MainScene->camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(0.0, -7.0, 5.0), centriod, glm::vec3(0.0,0.0,1.0));
	System::MainScene->entities->addObject("camera", System::MainScene->camera);
		
	/* Create an entity with the provided mesh and model */
	std::shared_ptr<Entities::Model> redTeapot = make_shared<Model>();
	redTeapot->transform.SetPosition(-1.5, 0.0, 0.0);
	redTeapot->transform.SetScale(.1, .1, .1);
	redTeapot->transform.SetRotation(-3.0, glm::vec3(0.0, 0.0, 1.0));
	redTeapot->setMesh(MeshList["mesh"]);
	redTeapot->addMaterial(redSurface);
	System::MainScene->entities->addObject("redTeapot", redTeapot);

	std::shared_ptr<Entities::Model> greenTeapot = make_shared<Model>();
	greenTeapot->transform.SetPosition(0.0, -1.5, 0.0);
	greenTeapot->transform.SetScale(.1, .1, .1);
	greenTeapot->transform.SetRotation(-2.0, glm::vec3(0.0, 0.0, 1.0));
	greenTeapot->setMesh(MeshList["mesh"]);
	greenTeapot->addMaterial(greenSurface);
	System::MainScene->entities->addObject("greenTeapot", greenTeapot);
	
	std::shared_ptr<Entities::Model> blueTeapot = make_shared<Model>();
	blueTeapot->transform.SetPosition(1.5, 0.0, 0.0);
	blueTeapot->transform.SetScale(.1, .1, .1);
	blueTeapot->transform.SetRotation(-0.3, glm::vec3(0.0, 0.0, 1.0));
	blueTeapot->setMesh(MeshList["mesh"]);
	blueTeapot->addMaterial(blueSurface);
	System::MainScene->entities->addObject("blueTeapot", blueTeapot);

	std::shared_ptr<Entities::Model> plane = make_shared<Model>();
	plane->setMesh(MeshList["plane"]);
	plane->transform.SetScale(4.0, 4.0, 4.0);
	plane->addMaterial(planeSurface);
	System::MainScene->entities->addObject("plane", plane);
		
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

	Components::Materials::SurfaceMaterials::UniformColoredSurface::Destroy();
	Components::Materials::SurfaceMaterials::BlinnSurface::Destroy();
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
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 3 - Shading", false, false, true };
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