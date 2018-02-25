#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/SpinTableCamera.hpp"
#include "Entities/Cameras/PerspectiveCamera.hpp"
#include "Entities/Lights/Light.hpp"

#include "Components/Meshes/Mesh.hpp"

#include "Components/Textures/Texture2D.hpp"

using namespace std;

void System::RenderLoop() {
	using namespace VKDK;
	auto lastTime = glfwGetTime();

	/* Record the commands required to render the current scene. */
	System::MainScene->subScenes["SubScene"]->setClearColor(glm::vec4(0.3, 0.3, 0.3, 1.0));
	System::MainScene->recordRenderPass();

	int oldWidth = VKDK::CurrentWindowSize[0], oldHeight = VKDK::CurrentWindowSize[1];
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();
		/* Aquire a new image from the swapchain */
		if (PrepareFrame() == true) System::MainScene->recordRenderPass();

		/* Submit offscreen pass to graphics queue */
		Scene::SubmitToGraphicsQueueInfo offscreenPassInfo;
		offscreenPassInfo.graphicsQueue = VKDK::graphicsQueue;
		offscreenPassInfo.commandBuffers = { MainScene->subScenes["SubScene"]->getCommandBuffer() };
		offscreenPassInfo.waitSemaphores = { VKDK::semaphores.presentComplete };
		offscreenPassInfo.signalSemaphores = { VKDK::semaphores.offscreenComplete };
		System::MainScene->submitToGraphicsQueue(offscreenPassInfo);

		/* Submit final pass to graphics queue  */
		Scene::SubmitToGraphicsQueueInfo finalPassInfo;
		finalPassInfo.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
		finalPassInfo.graphicsQueue = VKDK::graphicsQueue;
		finalPassInfo.waitSemaphores = { VKDK::semaphores.offscreenComplete };
		finalPassInfo.signalSemaphores = { VKDK::semaphores.renderComplete };
		System::MainScene->submitToGraphicsQueue(finalPassInfo);

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

	/* Initialize Materials */
	Materials::SurfaceMaterials::NormalSurface::Initialize(10, {System::MainScene->getRenderPass(), MainScene->subScenes["SubScene"]->getRenderPass()});
	Materials::SurfaceMaterials::TexCoordSurface::Initialize(10, { System::MainScene->getRenderPass(), MainScene->subScenes["SubScene"]->getRenderPass() });
	Materials::SurfaceMaterials::UniformColoredSurface::Initialize(10, { System::MainScene->getRenderPass(), MainScene->subScenes["SubScene"]->getRenderPass() });
	Materials::SurfaceMaterials::BlinnSurface::Initialize(10, { System::MainScene->getRenderPass(), MainScene->subScenes["SubScene"]->getRenderPass() });
	Materials::SurfaceMaterials::TexturedBlinnSurface::Initialize(10, { System::MainScene->getRenderPass(), MainScene->subScenes["SubScene"]->getRenderPass() });

	/* Load a test texture */
	TextureList["ChickenTexture"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/ChickenTexture.ktx");
	TextureList["ChickenTexture_s"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/ChickenTexture_s.ktx");
	TextureList["GrassTexture"] = make_shared<Textures::Texture2D>(ResourcePath "Chicken/grass.ktx");

	/* Load the model (by default, a teapot) */
	MeshList["Body"] = make_shared<Meshes::OBJMesh>(ResourcePath "Chicken/Chicken.obj");
	MeshList["Eyes"] = make_shared<Meshes::OBJMesh>(ResourcePath "Chicken/Eyes.obj");
	MeshList["plane"] = make_shared<Meshes::Plane>();
	MeshList["sphere"] = make_shared<Meshes::Sphere>();
}

double lastSubsceneX, lastSubsceneY, subsceneX, subsceneY;
void UpdateSubSceneLight(Entities::Entity *light) {
	glfwGetCursorPos(VKDK::DefaultWindow, &subsceneX, &subsceneY);
	if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT) && (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL))) {
		if (!glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT)) return;
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), subsceneX - lastSubsceneX);
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0), subsceneY - lastSubsceneY);
	}
	else {
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), .2f);
	}
	lastSubsceneX = subsceneX;
	lastSubsceneY = subsceneY;
}

double lastX, lastY, X, Y;
void UpdateLight(Entities::Entity *light) {

	glfwGetCursorPos(VKDK::DefaultWindow, &X, &Y);
	if (!glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT) && (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL))) {
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), X - lastX);
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0), Y - lastY);
	}
	else {
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), .2f);
	}
	lastX = X;
	lastY = Y;
}

void System::SetupEntities() {
	using namespace std;
	using namespace Entities;
	using namespace Components::Materials;

	/* Light colors */
	glm::vec4 la = glm::vec4(.1, .1, .1, 1.0);
	glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0);
	glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0);

	/* ---------------------------
		MAIN SCENE
	-----------------------------*/

	/* Create an orbit camera to look at the plane */
	MainScene->camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(0.0, -5.0, 0.0), glm::vec3(0.0,0.0,0.0), glm::vec3(0.0, 0.0, 1.0));
	MainScene->entities->addObject("camera", MainScene->camera);
	
	/* Front Plane */
	auto SubSceneMaterial1 = make_shared<SurfaceMaterials::TexturedBlinnSurface>(MainScene, TextureList["SubSceneTexture"]);
	SubSceneMaterial1->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(.8, .8, .8, 1.0), glm::vec4(.0, .0, .0, 0.0));
	std::shared_ptr<Entities::Model> FrontPlane = make_shared<Model>();
	FrontPlane->transform.SetScale(1.0, 1.0, 1.0);
	FrontPlane->transform.SetRotation(3.14/2.0, glm::vec3(1.0, 0.0, 0.0));
	FrontPlane->setMesh(MeshList["plane"]);
	FrontPlane->addMaterial(SubSceneMaterial1);
	MainScene->entities->addObject("FrontPlane", FrontPlane);

	/* Back Plane */
	auto SubSceneMaterial2 = make_shared<SurfaceMaterials::TexturedBlinnSurface>(MainScene, TextureList["SubSceneTexture"]);
	SubSceneMaterial2->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(.8, .8, .8, 1.0), glm::vec4(.0, .0, .0, 0.0));
	std::shared_ptr<Entities::Model> BackPlane = make_shared<Model>();
	BackPlane->transform.SetPosition(0.0, .01, 0.0);
	BackPlane->transform.SetScale(1.0, 1.0, 1.0);
	BackPlane->transform.SetRotation(3.14 / 2.0, glm::vec3(1.0, 0.0, 0.0));
	BackPlane->transform.AddRotation(3.14, glm::vec3(0.0, 1.0, 0.0));
	BackPlane->setMesh(MeshList["plane"]);
	BackPlane->addMaterial(SubSceneMaterial2);
	MainScene->entities->addObject("BackPlane", BackPlane);
	
	/* Light */
	auto mainLightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(MainScene);
	mainLightSurface->setColor(la + ld + ls);
	auto MainSceneLight = make_shared<Lights::Light>(la, ld, ls);
	MainSceneLight->transform.SetPosition(glm::vec3(0.0, -1.5, 0.0));
	MainSceneLight->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
	MainSceneLight->addMaterial(mainLightSurface);
	MainSceneLight->setMesh(MeshList["sphere"]);
	MainSceneLight->updateCallback = &UpdateLight;
	MainScene->entities->addObject("light", MainSceneLight);
	MainScene->LightList.emplace("light", MainSceneLight);

	/* ---------------------------
		SUB SCENE
	-----------------------------*/
	std::shared_ptr<Scene> SubScene = MainScene->subScenes["SubScene"];

	/* Create an orbit camera to look at the model */
	glm::vec3 centroid = MeshList["Body"]->getCentroid();
	centroid *= .2;
	SubScene->camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(3.0, -5.0, 3.0), centroid, glm::vec3(0.0, 0.0, 1.0), true);
	SubScene->entities->addObject("camera", SubScene->camera);

	/* Floor */
	auto FloorMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(SubScene, TextureList["GrassTexture"]);
	FloorMaterial->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(.8, .8, .8, 1.0), glm::vec4(.0, .0, .0, 0.0));
	std::shared_ptr<Entities::Model> Floor = make_shared<Model>();
	Floor->transform.SetScale(5.0, 5.0, 5.0);
	Floor->setMesh(MeshList["plane"]);
	Floor->addMaterial(FloorMaterial);
	SubScene->entities->addObject("Floor", Floor);

	/* Chicken Body */
	auto chickenMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(SubScene, TextureList["ChickenTexture"], TextureList["ChickenTexture_s"]);
	std::shared_ptr<Entities::Model> Body = make_shared<Model>();
	Body->transform.SetScale(.2, .2, .2);
	Body->setMesh(MeshList["Body"]);
	Body->addMaterial(chickenMaterial);
	SubScene->entities->addObject("Base", Body);
	
	/* Chicken Eyes */
	auto eyeMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(SubScene, TextureList["ChickenTexture"]);
	std::shared_ptr<Entities::Model> Eyes = make_shared<Model>();
	Eyes->transform.SetScale(.2, .2, .2);
	Eyes->setMesh(MeshList["Eyes"]);
	Eyes->addMaterial(eyeMaterial);
	SubScene->entities->addObject("Eyes", Eyes);

	/* Subscene light */
	auto SubLightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(SubScene);
	SubLightSurface->setColor(la + ld + ls);
	auto SubSceneLight = make_shared<Lights::Light>(la, ld, ls);
	SubSceneLight->transform.SetPosition(glm::vec3(0.0, -3.0, 3.0));
	SubSceneLight->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
	SubSceneLight->addMaterial(SubLightSurface);
	SubSceneLight->setMesh(MeshList["sphere"]);
	SubSceneLight->updateCallback = &UpdateSubSceneLight;
	SubScene->entities->addObject("subscenelight", SubSceneLight);
	SubScene->LightList.emplace("subscenelight", SubSceneLight);
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

	for (auto &mesh : MeshList) { 
		mesh.second->cleanup(); 
	}
	for (auto &tex : TextureList) { 
		tex.second->cleanup(); 
	}
	
	Components::Materials::SurfaceMaterials::NormalSurface::Destroy();
	Components::Materials::SurfaceMaterials::TexCoordSurface::Destroy();
	Components::Materials::SurfaceMaterials::UniformColoredSurface::Destroy();
	Components::Materials::SurfaceMaterials::BlinnSurface::Destroy();
	Components::Materials::SurfaceMaterials::TexturedBlinnSurface::Destroy();

	using namespace VKDK;
}

void System::Initialize() {
	MainScene = make_shared<Scene>(true);

	/* New: create a sub-scene that we can use as a texture */
	MainScene->subScenes.emplace("SubScene", make_shared<Scene>(false, "SubSceneTexture", 1024, 1024));
}

void System::Terminate() {
	MainScene->cleanup();
}

int main(int argc, char** argv) {
	/* Process arguments */
	Options::ProcessArgs(argc, argv);

	/* Initialize Vulkan */
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 5 - Render Buffers", false, false, true };
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