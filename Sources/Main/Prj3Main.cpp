#include "Options/Options.h"
#include "System/System.hpp"
#include "glm/glm.hpp"

#include "Entities/Model/Model.hpp"
#include "Entities/Cameras/OrbitCamera.hpp"
#include "Entities/Lights/Light.hpp"

#include "Components/Meshes/Mesh.hpp"
using namespace std;

void System::UpdateLoop() {
	auto lastTime = glfwGetTime();
	while (quit == false) {
		auto currentTime = glfwGetTime();
		if (currentTime - lastTime > 1.0 / UpdateRate) {
			/* Update Light Buffer */
			System::UpdateLightBuffer();

			/* Update Camera Buffer */
			System::UpdateCameraBuffer();

			World.update();
			lastTime = currentTime;
		}
	}
}

void System::RenderLoop() {
	shared_ptr<Entities::Cameras::OrbitCamera> camera =
		dynamic_pointer_cast<Entities::Cameras::OrbitCamera>(World.children.at("camera"));

	int oldWidth = VKDK::DefaultWindowSize[0], oldHeight = VKDK::DefaultWindowSize[1];
	auto lastTime = glfwGetTime();

	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();

		glfwPollEvents();
		
		/* If the window resized, then the swapchain updated, so we need to update the material pipelines */
		int width, height;
		glfwGetWindowSize(VKDK::DefaultWindow, &width, &height);
		if (oldWidth != width || oldHeight != height) {
			oldWidth = width; oldHeight = height;
			camera->setWindowSize(width, height);
			Components::Materials::SurfaceMaterials::UniformColoredSurface::RefreshPipeline();
			Components::Materials::SurfaceMaterials::BlinnSurface::RefreshPipeline();
			continue;
		};

		/* Get current image index */
		currentImageIndex = VKDK::GetNextImageIndex();

		/* Go through the scene, recording render passes into the current command buffer */
		VKDK::StartCommandBufferRecording(currentImageIndex);
		
		System::World.render(glm::mat4(1.0), camera->getView(), camera->getProjection());

		/* Stop recording the command buffer */
		VKDK::StopCommandBufferRecording(currentImageIndex);

		/* Submit command buffer to graphics queue for rendering */
		VKDK::DrawFrame(currentImageIndex);

		std::cout << "\r Framerate: " << currentTime - lastTime;
		lastTime = currentTime;

	}

	vkDeviceWaitIdle(VKDK::device);
}

void System::SetupComponents() {
	using namespace Components;
	using namespace Materials;
	using namespace std;

	/* Initialize Materials */
	Materials::SurfaceMaterials::NormalSurface::Initialize(10);
	Materials::SurfaceMaterials::UniformColoredSurface::Initialize(10);
	Materials::SurfaceMaterials::BlinnSurface::Initialize(10);
	
	/* Load the model (by default, a teapot) */
	auto mesh = make_shared<Meshes::OBJMesh>();
	mesh->loadFromOBJ(Options::objLocation);
	MeshList.emplace("mesh", mesh);

	auto sphere = make_shared<Meshes::Sphere>();
	MeshList.emplace("sphere", sphere);

	auto plane = make_shared<Meshes::Plane>();
	MeshList.emplace("plane", plane);
}

double lastx, lasty, x, y;
void UpdateLight(Entities::Entity *light) {
	glfwGetCursorPos(VKDK::DefaultWindow, &x, &y);
	if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) {

		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), x - lastx);
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0) , y - lasty);
	}
	else {
		light->transform.RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), .1f);
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

	auto redSurface = make_shared<SurfaceMaterials::BlinnSurface>();
	auto greenSurface = make_shared<SurfaceMaterials::BlinnSurface>();
	auto blueSurface = make_shared<SurfaceMaterials::BlinnSurface>();
	auto planeSurface = make_shared<SurfaceMaterials::BlinnSurface>();

	redSurface->setColor(red, red, white);
	greenSurface->setColor(green, green, white);
	blueSurface->setColor(blue, blue, white);
	planeSurface->setColor(grey, grey, white);
	
	glm::vec4 la = glm::vec4(.1, .1, .1, 1.0);
	glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0);
	glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0);
	auto lightSurface = make_shared<SurfaceMaterials::UniformColoredSurface>();
	lightSurface->setColor(la + ld + ls);

	/* Create an orbit camera to look at the model */
	glm::vec3 centriod = MeshList["mesh"]->getCentroid();
	centriod *= .1;
	System::camera = make_shared<Cameras::OrbitCamera>(glm::vec3(0.0, -7.0, 5.0), centriod);
	World.addObject("camera", System::camera);
		
	/* Create an entity with the provided mesh and model */
	std::shared_ptr<Entities::Model> redTeapot = make_shared<Model>();
	redTeapot->transform.SetPosition(-1.5, 0.0, 0.0);
	redTeapot->transform.SetScale(.1, .1, .1);
	redTeapot->transform.SetRotation(-3.0, glm::vec3(0.0, 0.0, 1.0));
	redTeapot->setMesh(MeshList["mesh"]);
	redTeapot->addMaterial(redSurface);
	World.addObject("redTeapot", redTeapot);

	std::shared_ptr<Entities::Model> greenTeapot = make_shared<Model>();
	greenTeapot->transform.SetPosition(0.0, -1.5, 0.0);
	greenTeapot->transform.SetScale(.1, .1, .1);
	greenTeapot->transform.SetRotation(-2.0, glm::vec3(0.0, 0.0, 1.0));
	greenTeapot->setMesh(MeshList["mesh"]);
	greenTeapot->addMaterial(greenSurface);
	World.addObject("greenTeapot", greenTeapot);
	
	std::shared_ptr<Entities::Model> blueTeapot = make_shared<Model>();
	blueTeapot->transform.SetPosition(1.5, 0.0, 0.0);
	blueTeapot->transform.SetScale(.1, .1, .1);
	blueTeapot->transform.SetRotation(-0.3, glm::vec3(0.0, 0.0, 1.0));
	blueTeapot->setMesh(MeshList["mesh"]);
	blueTeapot->addMaterial(blueSurface);
	World.addObject("blueTeapot", blueTeapot);

	std::shared_ptr<Entities::Model> plane = make_shared<Model>();
	plane->setMesh(MeshList["plane"]);
	plane->transform.SetScale(4.0, 4.0, 4.0);
	plane->addMaterial(planeSurface);
	World.addObject("plane", plane);
		
	/* Add a light to the scene */
	auto light = make_shared<Lights::Light>(la, ld, ls);
	light->transform.SetPosition(glm::vec3(0.0, -3.0, 3.0));
	light->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
	light->addMaterial(lightSurface);
	light->setMesh(MeshList["sphere"]);
	light->updateCallback = &UpdateLight;
	System::World.addObject("light", light);
	System::LightList.emplace("light", light);
}

void System::Start() {
	if (System::quit) return;
	
	/* Update on seperate thread */
	System::UpdateThread = new std::thread(System::UpdateLoop);

	/* Render on the current thread */
	System::RenderLoop();
}

void System::Cleanup() {
	/* Quit */
	System::quit = true;

	if (System::UpdateThread) {
		System::UpdateThread->join();
		delete(System::UpdateThread);
	}

	for (auto &mesh : MeshList) { mesh.second->cleanup(); }
	System::World.cleanup();

	Components::Materials::SurfaceMaterials::NormalSurface::Destroy();
	Components::Materials::SurfaceMaterials::UniformColoredSurface::Destroy();
	Components::Materials::SurfaceMaterials::BlinnSurface::Destroy();

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