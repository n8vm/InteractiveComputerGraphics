#include "vkdk.hpp"
#include "glm/glm.hpp"
#include "ecs.hpp"

namespace E = Entities;
namespace C = Components;
namespace S = Systems;

namespace CM = Systems::ComponentManager;
namespace SG = Systems::SceneGraph;

namespace Math = Components::Math;
namespace Lights = Components::Lights;
namespace Materials = Components::Materials::Standard;
namespace Meshes = Components::Meshes;
namespace Cameras = Entities::Cameras;
namespace Textures = Components::Textures;

using namespace std;
//
//void System::RenderLoop() {
//	using namespace VKDK;
//	auto lastTime = glfwGetTime();
//
//	/* Record the commands required to render the current scene. */
//	System::MainScene->subScenes["SubScene"]->setClearColor(glm::vec4(0.3, 0.3, 0.3, 1.0));
//	System::MainScene->recordRenderPass();
//
//	int oldWidth = VKDK::CurrentWindowSize[0], oldHeight = VKDK::CurrentWindowSize[1];
//	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
//		auto currentTime = glfwGetTime();
//		
//		/* Aquire a new image from the swapchain */
//		if (PrepareFrame() == true) System::MainScene->recordRenderPass();
//
//		/* Submit offscreen pass to graphics queue */
//		Scene::SubmitToGraphicsQueueInfo offscreenPassInfo;
//		offscreenPassInfo.graphicsQueue = VKDK::graphicsQueue;
//		offscreenPassInfo.commandBuffers = { MainScene->subScenes["SubScene"]->getCommandBuffer() };
//		offscreenPassInfo.waitSemaphores = { VKDK::semaphores.presentComplete };
//		offscreenPassInfo.signalSemaphores = { VKDK::semaphores.offscreenComplete };
//		System::MainScene->submitToGraphicsQueue(offscreenPassInfo);
//
//		/* Submit final pass to graphics queue  */
//		Scene::SubmitToGraphicsQueueInfo finalPassInfo;
//		finalPassInfo.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
//		finalPassInfo.graphicsQueue = VKDK::graphicsQueue;
//		finalPassInfo.waitSemaphores = { VKDK::semaphores.offscreenComplete };
//		finalPassInfo.signalSemaphores = { VKDK::semaphores.renderComplete };
//		System::MainScene->submitToGraphicsQueue(finalPassInfo);
//
//		/* Submit the frame for presenting. */
//		if (SubmitFrame() == true) System::MainScene->recordRenderPass();
//
//		std::cout << "\r Framerate: " << currentTime - lastTime;
//		lastTime = currentTime;
//	}
//	vkDeviceWaitIdle(device);
//}
//
//void System::EventLoop() {
//	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
//		glfwPollEvents();
//	}
//}
//
//void System::UpdateLoop() {
//	auto lastTime = glfwGetTime();
//	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
//		auto currentTime = glfwGetTime();
//		if (currentTime - lastTime > 1.0 / UpdateRate) {
//			lastTime = currentTime;
//			/* Update Camera Buffer */
//			System::MainScene->updateCameraUBO();
//			System::MainScene->update();
//		}
//	}
//}
//


//double lastx, lasty, x, y;
//std::shared_ptr<Entities::Model> mirror;
//void UpdateReflector(Entities::Entity *reflector) {
//	glfwGetCursorPos(VKDK::DefaultWindow, &x, &y);
//	if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) {
//
//		mirror->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.up, x - lastx);
//		mirror->transform.RotateAround(glm::vec3(0.0), System::MainScene->camera->transform.right, y - lasty);
//	}
//	else {
//		mirror->transform.RotateAround(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), .1);
//	}
//	lastx = x;
//	lasty = y;
//
//	/* Reflecting accross the mirror */
//	glm::vec3 V = glm::normalize(mirror->transform.forward.load());// glm::vec3(0.0, 0.0, 1.0);
//	glm::vec3 P = mirror->transform.position;
//	glm::mat4 reflectionMatrix = glm::mat4(
//		1.0 - 2 * V.x * V.x     , -2 * V.x * V.y           , -2 * V.x * V.z,           0.0,
//		-2 * V.x * V.y          , 1.0 - 2 * V.y * V.y      , -2 * V.y * V.z,           0.0,
//		-2 * V.x * V.z          , -2 * V.y * V.z           , 1.0 - 2 * V.z * V.z,      0.0,
//		2 * glm::dot(P, V) * V.x, 2 * glm::dot(P, V) * V.y , 2 * glm::dot(P, V) * V.z, 1.0
//	);
//
//	glm::mat4 invRefl = glm::transpose(glm::inverse(reflectionMatrix));
//	glm::vec4 norm = glm::vec4(0.0, 0.0, 1.0, 0.0);
//	glm::vec4 reversed = invRefl * norm;
//
//	System::MainScene->subScenes["SubScene"]->SceneTransform = reflectionMatrix;
//}

namespace PRJ6 {
	void SetupComponents() {
	  CM::Initialize();

	  /* Load Meshes */
	  auto teapot = Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");
	  glm::vec3 centroid = teapot->mesh->getCentroid();

	  /* Load Skybox (Example where texture compression could be used) */
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

	  Textures::TextureCube::Create("SkyboxTexture", texpath);

	  /* Load Grass */
	  Textures::Texture2D::Create("GrassTexture", ResourcePath "SkyboxTextures/Cem/floor.ktx");

	  /* Setup perspective for a reflection texture */
	  auto P1 = Math::Perspective::Create("P1", 1024, 1024);
	  //P1->view = glm::lookAt(glm::vec3(0.0, -30.0, 12.0), centroid, glm::vec3(0.0, 0.0, 1.0));

	  /* Perspective for the final render Pass  */
	  auto P2 = Math::Perspective::Create("P2",
		VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
		VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

	  auto P1_CCW_Key = PipelineKey(P1->renderpass, 0, 0);
	  auto P1_CW_Key = PipelineKey(P1->renderpass, 0, 0);
	  auto P1_NOCULL_Key = PipelineKey(P1->renderpass, 0, 0);
	  auto P2_CCW_Key = PipelineKey(P2->renderpass, 0, 0);
	  auto P2_CW_Key = PipelineKey(P2->renderpass, 0, 0);
	  auto P2_NOCULL_Key = PipelineKey(P2->renderpass, 0, 0);

	  auto P1_CCW_Params = PipelineParameters::Create(P1_CCW_Key);
	  auto P1_CW_Params = PipelineParameters::Create(P1_CW_Key);
	  auto P1_NOCULL_Params = PipelineParameters::Create(P1_NOCULL_Key);
	  auto P2_CCW_Params = PipelineParameters::Create(P2_CCW_Key);
	  auto P2_CW_Params = PipelineParameters::Create(P2_CW_Key);
	  auto P2_NOCULL_Params = PipelineParameters::Create(P2_NOCULL_Key);

	  P1_CCW_Params->rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	  P1_CW_Params->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	  P1_NOCULL_Params->rasterizer.cullMode = VK_CULL_MODE_NONE;
	  P2_CCW_Params->rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	  P2_CW_Params->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	  P2_NOCULL_Params->rasterizer.cullMode = VK_CULL_MODE_NONE;

	  /* Initialize Materials */
	  Materials::Skybox::Initialize(10);
	  Materials::Blinn::Initialize(25);
	  Materials::UniformColor::Initialize(25);

	  /* Render the skybox inside out */
	  auto skymat = Materials::Skybox::Create("SkyboxMaterial", P2_CW_Key);
	  skymat->useTexture(true);
	  skymat->setTexture(CM::Textures["SkyboxTexture"]);

	  /* For all other materials, account for reverse winding in the first render pass. */
	  auto blinnMat = Materials::Blinn::Create("BlinnMat", P2_CCW_Key);
	  blinnMat->useCubemapTexture(true);
	  blinnMat->setCubemapTexture(CM::Textures["SkyboxTexture"]);
	  blinnMat->setColor(Colors::black, Colors::white, Colors::black, glm::vec4(.1,.1,.1,1.0));
	  //blinnMat->useRoughness(true);
	  //blinnMat->setRoughness(7.5);

	  auto uniformColMat = Materials::UniformColor::Create("UniformColor", P2_CCW_Key);
	  uniformColMat->setColor(Colors::white);

	  auto grassmat = Materials::UniformColor::Create("GrassMaterial", P2_CCW_Key);
	  grassmat->useTexture(true);
	  grassmat->setTexture(CM::Textures["GrassTexture"]);

	  //Materials::UniformColor::Create("SkyboxMaterial", P2_CW_Key, CM::Textures["GrassTexture"]);

  

	  //Materials::SurfaceMaterials::BlinnSurface::Initialize(10, { P1_CW, P2_CCW });
	  //Materials::SurfaceMaterials::TexturedBlinnSurface::Initialize(10, { P1_NOCULL, P2_NOCULL });
	  //Materials::SurfaceMaterials::CubemapReflectionSurface::Initialize(10, { P1_CW, P2_CCW });
	  //Materials::SurfaceMaterials::EyeSpaceReflectionSurface::Initialize(10, { P1_CW, P2_CCW });
	  //Materials::SurfaceMaterials::BlinnReflectionSurface::Initialize(10, { P1_CW, P2_CCW });

	  auto l1 = Lights::PointLights::Create("Light");

	}

	void SetupEntities() {
	  /* Create an orbit camera to look at the model */
	  auto centriod = CM::Meshes["Teapot"]->mesh->getCentroid();
	  centriod *= .1f;
	  auto camera = Cameras::SpinTableCamera::Create("Camera",
		glm::vec3(0.0f, -7.0f, 5.0f), glm::vec3(0.0f, 0.0f, centriod.z), glm::vec3(0.0f, 0.0f, 1.0f));
	  camera->addComponent(CM::Perspectives["P2"]);
	  camera->setZoomAcceleration(.05);

	  /* Teapot */
	  auto Teapot = E::Entity::Create("Teapot");
	  Teapot->addComponent(CM::Materials["BlinnMat"]);
	  Teapot->addComponent(CM::Meshes["Teapot"]);
	  Teapot->transform->SetScale(.2f, .2f, .2f);

	  /* Floor */
	  auto Floor = E::Entity::Create("Floor");
	  Floor->addComponent(CM::Materials["GrassMaterial"]);
	  Floor->addComponent(CM::Meshes["Plane"]);
	  Floor->transform->SetScale(5.0f, 5.0f, 5.0f);
  
	  /* Light */
	  auto Light = E::Entity::Create("Light");
	  Light->addComponent(CM::Meshes["Sphere"]);
	  Light->addComponent(CM::Materials["UniformColor"]);
	  Light->addComponent(CM::Lights["Light"]);
	  Light->transform->SetPosition(glm::vec3(0.0, -3.0, 3.0));
	  Light->transform->SetScale(glm::vec3(0.1, 0.1, 0.1));
	  Light->callbacks->update = [](std::shared_ptr<E::Entity> Light) {
   		Light->transform->RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), -.2f);
	  };

	  /* Skybox */
	  auto Skybox = E::Entity::Create("Skybox");
	  Skybox->addComponent(CM::Meshes["Sphere"]);
	  Skybox->addComponent(CM::Materials["SkyboxMaterial"]);
	  Skybox->transform->SetScale(100.0f, 100.0f, 100.0f);

	  //auto skyboxMaterial = make_shared<SurfaceMaterials::SkyboxSurface>(scenes, TextureList["Skybox"]);
	  //std::shared_ptr<Entities::Model> skybox = make_shared<Model>();
	  //skybox->transform.SetScale(100, 100, 100);
	  //skybox->setMesh(MeshList["sphere"]);
	  //skybox->addMaterial(skyboxMaterial);
	  //MainScene->entities->addObject("Skybox", skybox);
	  //SubScene->entities->addObject("Skybox", skybox);

	  //using namespace std;
	  //using namespace Entities;
	  //using namespace Components::Materials;

	  //std::shared_ptr<Scene> SubScene = MainScene->subScenes["SubScene"];

	  //std::vector<std::shared_ptr<Scene>> scenes = { MainScene, SubScene };

	  ///* Light colors */
	  //glm::vec4 la = glm::vec4(.5, .5, .5, 1.0);
	  //glm::vec4 ld = glm::vec4(.8, .8, .8, 1.0) * 5.f;
	  //glm::vec4 ls = glm::vec4(.8, .8, .8, 1.0) * 5.f;

	  ///* Create an orbit camera to look at the model */
	  //glm::vec3 centriod = MeshList["Teapot"]->getCentroid();
	  //centriod *= .2;
	  //auto camera = make_shared<Cameras::SpinTableCamera>(glm::vec3(5.0, -7.0, 5.0), centriod, glm::vec3(0.0, 0.0, 1.0));
	  //camera->setWindowSize(VKDK::CurrentWindowSize[0], VKDK::CurrentWindowSize[1]);

	  ///* Cameras */
	  //System::MainScene->camera = camera;
	  //SubScene->camera = camera;
	  //System::MainScene->entities->addObject("camera", System::MainScene->camera);






	  ///* Teapot */
	  ////auto teapotMaterial = make_shared<SurfaceMaterials::CubemapReflectionSurface>(System::MainScene, TextureList["Skybox"]);
	  //auto teapotMaterial = make_shared<SurfaceMaterials::BlinnReflectionSurface>(scenes, TextureList["Skybox"]);
	  //teapotMaterial->setColor(glm::vec4(0.9, 0.9, 0.9, 1.0), glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(.9, .9, .9, 1.0));
	  //std::shared_ptr<Entities::Model> teapot = make_shared<Model>();
	  //teapot->transform.SetScale(.2, .2, .2);
	  //teapot->setMesh(MeshList["Teapot"]);
	  //teapot->addMaterial(teapotMaterial);
	  //MainScene->entities->addObject("Base", teapot);
	  //SubScene->entities->addObject("Base", teapot);


	  ///* Mirror (final renderpass only )*/
	  //auto mirrorTexture = make_shared<SurfaceMaterials::EyeSpaceReflectionSurface>(MainScene, TextureList["SubSceneTexture"]);
	  //mirrorTexture->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(.0, .0, .0, .0), glm::vec4(1.0, 1.0, 1.0, 1.0));
	  //mirror = make_shared<Model>();
	  //mirror->transform.SetScale(5.0, 8.0, 5.0);
	  //mirror->transform.SetPosition(0.0, 8.0, 0.0);
	  //mirror->transform.SetRotation(3.14 / 2.0, glm::vec3(1.0, 0.0, 0.0));
	  //mirror->setMesh(MeshList["plane"]);
	  //mirror->addMaterial(mirrorTexture);
	  //MainScene->entities->addObject("mirror", mirror);

	  ///* Update the reflector to move with the plane */
	  //SubScene->entities->updateCallback = UpdateReflector;
	}

	void SetupSystems() {
	  S::RenderSystem = []() {
		bool refreshRequired = false;
		auto lastTime = glfwGetTime();

		/* Take the perspective from the camera */
		auto perspective = CM::Perspectives["P2"];

		/* Record the commands required to render the current scene. */
		perspective->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));

		while (!VKDK::ShouldClose() && !Systems::quit) {
		  auto currentTime = glfwGetTime();

		  /* Upload Perspective UBOs before render */
		  for (auto pair : CM::Perspectives) {
			pair.second->uploadUBO();
		  }

		  /* Aquire a new image from the swapchain */
		  refreshRequired |= VKDK::PrepareFrame();

		  /* Submit to graphics queue  */
		  VKDK::SubmitToGraphicsQueueInfo info;
		  info.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
		  info.graphicsQueue = VKDK::graphicsQueue;
		  info.signalSemaphores = { VKDK::semaphores.renderComplete };
		  info.waitSemaphores = { VKDK::semaphores.presentComplete };
		  VKDK::SubmitToGraphicsQueue(info);

		  /* Submit the frame for presenting. */
		  refreshRequired |= VKDK::SubmitFrame();
		  if (refreshRequired) {
			VKDK::RecreateSwapChain();

			/* Add a perspective to render the swapchain */
			perspective->refresh(VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			  VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

			perspective->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
			refreshRequired = false;
		  }
		  std::cout << "\r Framerate: " << currentTime - lastTime;
		  lastTime = currentTime;
		}
		vkDeviceWaitIdle(VKDK::device);
	  };

	  S::EventSystem = []() {
		while (!VKDK::ShouldClose() && !Systems::quit) {
		  glfwPollEvents();
		}
	  };

	  S::UpdateSystem = []() {
		auto lastTime = glfwGetTime();
		while (!VKDK::ShouldClose() && !Systems::quit) {
		  auto currentTime = glfwGetTime();
		  if (currentTime - lastTime > 1.0 / S::UpdateRate) {
			/* Update Entities */
			for (auto pair : SG::Entities) {
			  if (pair.second->callbacks->update) {
				pair.second->callbacks->update(pair.second);
			  }
			}

			/* Upload Transform UBOs */
			for (auto pair : SG::Entities) {
			  glm::mat4 worldToLocal = pair.second->getWorldToLocalMatrix();
			  glm::mat4 localToWorld = glm::inverse(worldToLocal);
			  pair.second->transform->uploadUBO(worldToLocal, localToWorld);
			}

			/* Upload Material UBOs */
			for (auto pair : CM::Materials) {
			  pair.second->material->uploadUBO();
			}

			/* Upload Point Light UBO */
			Lights::PointLights::UploadUBO();

			lastTime = currentTime;
		  }
		}
	  };

	  S::currentThreadType = S::SystemTypes::Event;
	}

	void CleanupComponents() {
	  CM::Cleanup();

	  /* Destroy the requested material pipelines */
	  Materials::Blinn::Destroy();
	  Materials::UniformColor::Destroy();
	  Materials::Skybox::Destroy();
	}
}

void StartDemo6() {
  VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 6 - Environment Mapping", false, false, true };
  if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

  PRJ6::SetupComponents();
  PRJ6::SetupEntities();
  PRJ6::SetupSystems();
  S::LaunchThreads();

  S::JoinThreads();
  PRJ6::CleanupComponents();
  VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo6(); }
#endif