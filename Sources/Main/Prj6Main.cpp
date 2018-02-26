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
			lastTime = currentTime;
			/* Update Camera Buffer */
			System::MainScene->updateCameraUBO();
			System::MainScene->update();
		}
	}
}

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	PipelineParameters RP1_CCW = {};
	RP1_CCW.renderPass = MainScene->subScenes["SubScene"]->getRenderPass();
	RP1_CCW.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	PipelineParameters RP1_CW = {};
	RP1_CW.renderPass = MainScene->subScenes["SubScene"]->getRenderPass();
	RP1_CW.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	PipelineParameters RP1_NOCULL = {};
	RP1_NOCULL.renderPass = MainScene->subScenes["SubScene"]->getRenderPass();
	RP1_NOCULL.rasterizer.cullMode = VK_CULL_MODE_NONE;

	PipelineParameters RP2_CCW = {};
	RP2_CCW.renderPass = MainScene->getRenderPass();
	RP2_CCW.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	PipelineParameters RP2_CW = {};
	RP2_CW.renderPass = MainScene->getRenderPass();
	RP2_CW.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	PipelineParameters RP2_NOCULL = {};
	RP2_NOCULL.renderPass = MainScene->getRenderPass();
	RP2_NOCULL.rasterizer.cullMode = VK_CULL_MODE_NONE;

	/* Initialize Materials */
	
	/* Render the skybox inside out */
	Materials::SurfaceMaterials::SkyboxSurface::Initialize(10, { RP1_CCW, RP2_CW });

	/* For all other materials, account for reverse winding in the first render pass. */
	Materials::SurfaceMaterials::TexCoordSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::NormalSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::UniformColoredSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::BlinnSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::TexturedBlinnSurface::Initialize(10, { RP1_NOCULL, RP2_NOCULL });
	Materials::SurfaceMaterials::CubemapReflectionSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::EyeSpaceReflectionSurface::Initialize(10, { RP1_CW, RP2_CCW });
	Materials::SurfaceMaterials::BlinnReflectionSurface::Initialize(10, { RP1_CW, RP2_CCW });


	/* Load Skybox */
	std::string texpath;;
	VkFormat compressionFormat;
	if (VKDK::deviceFeatures.textureCompressionASTC_LDR) {
		texpath = ResourcePath "SkyboxTextures/Cem/cubemap_etc2_unorm.ktx";
		compressionFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
	}
	else if (VKDK::deviceFeatures.textureCompressionETC2) {
		texpath = ResourcePath "SkyboxTextures/Cem/cubemap_ASTC8X8_unorm.ktx";
		compressionFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
	}
	else if (VKDK::deviceFeatures.textureCompressionBC) {
		texpath = ResourcePath "SkyboxTextures/Cem/cubemap_BC2_unorm.ktx";
		compressionFormat = VK_FORMAT_BC2_UNORM_BLOCK;
	}

	TextureList["Skybox"] = make_shared<Textures::TextureCube>(texpath, compressionFormat);
	
	/* Load Grass */
	TextureList["GrassTexture"] = make_shared<Textures::Texture2D>(ResourcePath "SkyboxTextures/Cem/floor.ktx");

	/* Load the model (by default, a teapot) */
	MeshList["Teapot"] = make_shared<Meshes::OBJMesh>(ResourcePath "Teapot/teapot.obj");
	MeshList["plane"] = make_shared<Meshes::Plane>();
	MeshList["sphere"] = make_shared<Meshes::Sphere>();
	MeshList["cube"] = make_shared<Meshes::Cube>();
}

void UpdateLight(Entities::Entity *light) {
	light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), -.2f);
}

double lastx, lasty, x, y;
std::shared_ptr<Entities::Model> mirror;
void UpdateReflector(Entities::Entity *reflector) {
	glfwGetCursorPos(VKDK::DefaultWindow, &x, &y);
	if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) {

		mirror->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.up, x - lastx);
		mirror->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.right, y - lasty);
	}
	else {
		mirror->transform.RotateAround(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), .1);
	}
	lastx = x;
	lasty = y;

	/* Reflecting accross the mirror */
	glm::vec3 V = glm::normalize(mirror->transform.forward.load());// glm::vec3(0.0, 0.0, 1.0);
	glm::vec3 P = mirror->transform.position;
	glm::mat4 reflectionMatrix = glm::mat4(
		1.0 - 2 * V.x * V.x     , -2 * V.x * V.y           , -2 * V.x * V.z,           0.0,
		-2 * V.x * V.y          , 1.0 - 2 * V.y * V.y      , -2 * V.y * V.z,           0.0,
		-2 * V.x * V.z          , -2 * V.y * V.z           , 1.0 - 2 * V.z * V.z,      0.0,
		2 * glm::dot(P, V) * V.x, 2 * glm::dot(P, V) * V.y , 2 * glm::dot(P, V) * V.z, 1.0
	);

	glm::mat4 invRefl = glm::transpose(glm::inverse(reflectionMatrix));
	glm::vec4 norm = glm::vec4(0.0, 0.0, 1.0, 0.0);
	glm::vec4 reversed = invRefl * norm;

	System::MainScene->subScenes["SubScene"]->SceneTransform = reflectionMatrix;
}

void System::SetupEntities() {
	using namespace std;
	using namespace Entities;
	using namespace Components::Materials;

	/* Light colors */
	glm::vec4 la = glm::vec4(.5, .5, .5, 1.0);
	glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0);
	glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0);

	/* Create an orbit camera to look at the model */
	glm::vec3 centriod = MeshList["Teapot"]->getCentroid();
	centriod *= .2;
	auto camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(5.0, -7.0, 5.0), centriod, glm::vec3(0.0, 0.0, 1.0));
	
	/* ---------------------------
	MAIN SCENE
	-----------------------------*/
	{
		System::MainScene->camera = camera;
		System::MainScene->entities->addObject("camera", System::MainScene->camera);

		/* Skybox */
		auto skyboxMaterial = make_shared<SurfaceMaterials::SkyboxSurface>(System::MainScene, TextureList["Skybox"]);
		std::shared_ptr<Entities::Model> skybox = make_shared<Model>();
		skybox->transform.SetScale(100, 100, 100);
		skybox->setMesh(MeshList["sphere"]);
		skybox->addMaterial(skyboxMaterial);
		System::MainScene->entities->addObject("Skybox", skybox);
		
		/* Mirror */
		auto mirrorTexture = make_shared<SurfaceMaterials::EyeSpaceReflectionSurface>(MainScene, TextureList["SubSceneTexture"]);
		mirrorTexture->setColor(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0));
		mirror = make_shared<Model>();
		mirror->transform.SetScale(5.0, 8.0, 5.0);
		mirror->transform.SetPosition(0.0, 8.0, 0.0);
		mirror->transform.SetRotation(3.14 / 2.0, glm::vec3(1.0, 0.0, 0.0));
		mirror->setMesh(MeshList["plane"]);
		mirror->addMaterial(mirrorTexture);
		System::MainScene->entities->addObject("mirror", mirror);

		/* Floor */
		auto floorMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(System::MainScene, TextureList["GrassTexture"]);
		floorMaterial->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 0.0, 1.0));
		std::shared_ptr<Entities::Model> Floor = make_shared<Model>();
		Floor->transform.SetScale(5.0, 5.0, 5.0);
		Floor->setMesh(MeshList["plane"]);
		Floor->addMaterial(floorMaterial);
		System::MainScene->entities->addObject("Floor", Floor);

		/* Teapot */
		//auto teapotMaterial = make_shared<SurfaceMaterials::CubemapReflectionSurface>(System::MainScene, TextureList["Skybox"]);
		auto teapotMaterial = make_shared<SurfaceMaterials::BlinnReflectionSurface>(System::MainScene, TextureList["Skybox"]);
		teapotMaterial->setColor(glm::vec4(0.9, 0.9, 0.9, 1.0), glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(.9, .9, .9, 1.0));


		std::shared_ptr<Entities::Model> teapot = make_shared<Model>();
		teapot->transform.SetScale(.2, .2, .2);
		teapot->setMesh(MeshList["Teapot"]);
		teapot->addMaterial(teapotMaterial);
		System::MainScene->entities->addObject("Base", teapot);

		/* Add a light to the scene */
		auto lightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(System::MainScene);
		lightSurface->setColor(la + ld + ls);
		auto light = make_shared<Lights::Light>(la, ld, ls);
		light->transform.SetPosition(glm::vec3(0.0, -3.0, 3.0));
		light->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		light->addMaterial(lightSurface);
		light->setMesh(MeshList["sphere"]);
		light->updateCallback = &UpdateLight;
		System::MainScene->entities->addObject("light", light);
		System::MainScene->LightList.emplace("light", light);
	}


	/* ---------------------------
	SUB SCENE
	-----------------------------*/
	{
		std::shared_ptr<Scene> SubScene = MainScene->subScenes["SubScene"];

		/* This reflector scene uses the same camera as the main scene */
		SubScene->camera = camera;

		/* Skybox */
		auto skyboxMaterial = make_shared<SurfaceMaterials::SkyboxSurface>(SubScene, TextureList["Skybox"]);
		std::shared_ptr<Entities::Model> skybox = make_shared<Model>();
		skybox->transform.SetScale(100, 100, 100);
		skybox->setMesh(MeshList["sphere"]);
		skybox->addMaterial(skyboxMaterial);
		SubScene->entities->addObject("Skybox", skybox);

		/* Floor */
		auto floorMaterial = make_shared<SurfaceMaterials::TexturedBlinnSurface>(SubScene, TextureList["GrassTexture"]);
		floorMaterial->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 0.0, 1.0));
		std::shared_ptr<Entities::Model> Floor = make_shared<Model>();
		Floor->transform.SetScale(5.0, 5.0, 5.0);
		Floor->setMesh(MeshList["plane"]);
		Floor->addMaterial(floorMaterial);
		SubScene->entities->addObject("Floor", Floor);

		/* Teapot */
		//auto teapotMaterial = make_shared<SurfaceMaterials::CubemapReflectionSurface>(SubScene, TextureList["Skybox"]);
		auto teapotMaterial = make_shared<SurfaceMaterials::BlinnReflectionSurface>(SubScene, TextureList["Skybox"]);
		teapotMaterial->setColor(glm::vec4(0.9, 0.9, 0.9, 1.0), glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(.9, .9, .9, 1.0));

		std::shared_ptr<Entities::Model> teapot = make_shared<Model>();
		teapot->transform.SetScale(.2, .2, .2);
		teapot->setMesh(MeshList["Teapot"]);
		teapot->addMaterial(teapotMaterial);
		SubScene->entities->addObject("Base", teapot);

		/* Add a light to the scene */
		auto lightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>(SubScene);
		lightSurface->setColor(la + ld + ls);
		auto light = make_shared<Lights::Light>(la, ld, ls);
		light->transform.SetPosition(glm::vec3(0.0, -3.0, 3.0));
		light->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		light->addMaterial(lightSurface);
		light->setMesh(MeshList["sphere"]);
		light->updateCallback = &UpdateLight;
		SubScene->entities->addObject("light", light);
		SubScene->LightList.emplace("light", light);

		/* Update the reflector to move with the plane */
		SubScene->entities->updateCallback = UpdateReflector;
	}	
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

	Components::Materials::SurfaceMaterials::NormalSurface::Destroy();
	Components::Materials::SurfaceMaterials::TexCoordSurface::Destroy();
	Components::Materials::SurfaceMaterials::UniformColoredSurface::Destroy();
	Components::Materials::SurfaceMaterials::BlinnSurface::Destroy();
	Components::Materials::SurfaceMaterials::TexturedBlinnSurface::Destroy();
	Components::Materials::SurfaceMaterials::SkyboxSurface::Destroy();
	Components::Materials::SurfaceMaterials::CubemapReflectionSurface::Destroy();
	Components::Materials::SurfaceMaterials::EyeSpaceReflectionSurface::Destroy();
	Components::Materials::SurfaceMaterials::BlinnReflectionSurface::Destroy();
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
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 6 - Environment Mapping", false, false, false };
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