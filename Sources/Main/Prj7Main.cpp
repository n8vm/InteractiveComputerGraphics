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

namespace PRJ7 {
	void SetupComponents() {
		Systems::ComponentManager::Initialize();

		/* Load meshes and textures */
		Meshes::OBJMesh::Create("CornellBox", ResourcePath "Cornell/Cube.obj");
		Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");
		Textures::Texture2D::Create("CornellTexture", ResourcePath "Cornell/texture.ktx");

		/* Setup perspectives */
		/* Light perspective */
		auto P0 = Math::Perspective::Create("P0", 1024, 1024, true);

		auto P1 = Math::Perspective::Create("P1",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* Initialize Materials */
		PipelineKey P0_0_0 = PipelineKey(P0->renderpass, 0, 0);
		PipelineKey P1_0_0 = PipelineKey(P1->renderpass, 0, 0);
		PipelineKey P1_0_1 = PipelineKey(P1->renderpass, 0, 1);
		auto P0_0_0_Params = PipelineParameters::Create(P0_0_0);
		auto P1_0_0_Params = PipelineParameters::Create(P1_0_0);
		auto P1_0_1_Params = PipelineParameters::Create(P1_0_1);
		P1_0_1_Params->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		P1_0_1_Params->rasterizer.cullMode = VK_CULL_MODE_NONE;

		Materials::UniformColor::Initialize(10);
		Materials::Blinn::Initialize(25);
		Materials::Skybox::Initialize(10);
		Materials::Shadow::Initialize(10);

		auto shadowMat = Materials::Shadow::Create("ShadowMaterial", P0_0_0);

		auto cornellmat1 = Materials::Blinn::Create("CornellMat1", P0_0_0);
		auto redmat1 = Materials::Blinn::Create("RedTeapot1", P0_0_0);
		auto greenmat1 = Materials::Blinn::Create("GreenBall1", P0_0_0);

		auto cornellmat = Materials::Blinn::Create("CornellMat", P1_0_0);
		auto redmat = Materials::Blinn::Create("RedTeapot", P1_0_0);
		auto greenmat = Materials::Blinn::Create("GreenBall", P1_0_0);
		auto whitemat = Materials::UniformColor::Create("LightMaterial", P1_0_0);
		auto whiteBlinn = Materials::Blinn::Create("White", P1_0_0);
		whiteBlinn->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0));
		whiteBlinn->useShadowMapTexture(true);
		whiteBlinn->setShadowMapTexture(CM::Textures["P0"]);

		cornellmat->useShadowMapTexture(true);
		redmat->useShadowMapTexture(true);
		greenmat->useShadowMapTexture(true);
		cornellmat->setShadowMapTexture(CM::Textures["P0"]);
		redmat->setShadowMapTexture(CM::Textures["P0"]);
		greenmat->setShadowMapTexture(CM::Textures["P0"]);

		cornellmat1->setColor(Colors::white, Colors::black);
		cornellmat1->useDiffuseTexture(true);
		cornellmat1->setDiffuseTexture(CM::Textures["CornellTexture"]);

		cornellmat->setColor(Colors::white, Colors::black);
		cornellmat->useDiffuseTexture(true);
		cornellmat->setDiffuseTexture(CM::Textures["CornellTexture"]);

		redmat1->setColor(Colors::red + glm::vec4(0.0, .1, .1, 0.0));
		greenmat1->setColor(Colors::green + glm::vec4(0.1, .0, .1, 0.0));

		redmat->setColor(Colors::red + glm::vec4(0.0, .1, .1, 0.0));
		greenmat->setColor(Colors::green + glm::vec4(0.1, .0, .1, 0.0));
		whitemat->setColor(Colors::white);

		/* Render the skybox inside out */
		auto skymat = Materials::Skybox::Create("SkyboxMaterial", P1_0_1);
		skymat->useTexture(true);
		skymat->setTexture(CM::Textures["P0"]);

		/* Add light */
		auto l = Lights::PointLights::Create("Light", 512);
		l->setFalloffType(Lights::FalloffType::LINEAR);
		l->setIntensity(35.f);

		l->setColor(glm::vec4(1., 0.9, 0.9, 1.0));
		//l->setColor(glm::vec4(.878, 0.722, 0.42, 1.0));

		l->setFalloffType(Lights::FalloffType::SQUARED);
		l->setIntensity(5.f);

	}

	void SetupEntities() {
		/* Create an orbit camera to look at the model */
		auto camera = Cameras::SpinTableCamera::Create("Camera", glm::vec3(0.0, 50.0f, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), false, glm::radians(30.f));
		camera->addComponent(CM::Perspectives["P1"]);
		camera->setZoomAcceleration(.1);

		/* Cornell Box */
		auto CornellBox = E::Entity::Create("CornellBox");
		CornellBox->addComponent(CM::Meshes["CornellBox"]);
		CornellBox->addComponent(CM::Materials["CornellMat"]);
		CornellBox->addComponent(CM::Materials["ShadowMaterial"]);
		CornellBox->transform->SetScale(9.5, 9.5, 9.5);
		CornellBox->transform->SetPosition(0.0, 0.0, 0.0);

		/* Left Box */
		auto LeftBox = E::Entity::Create("LeftBox");
		LeftBox->addComponent(CM::Meshes["Cube"]);
		LeftBox->addComponent(CM::Materials["White"]);
		LeftBox->addComponent(CM::Materials["ShadowMaterial"]);
		LeftBox->transform->SetScale(3., 3., 6.);
		LeftBox->transform->SetPosition(3., -3., -4.);
		LeftBox->transform->SetRotation(3.14 / 8.0, glm::vec3(0.0, 0.0, 1.0));

		/* Right Box */
		auto Right = E::Entity::Create("Right");
		Right->addComponent(CM::Meshes["Cube"]);
		Right->addComponent(CM::Materials["White"]);
		Right->addComponent(CM::Materials["ShadowMaterial"]);
		Right->transform->SetScale(3., 3., 3.);
		Right->transform->SetPosition(-4., 4., -7.);
		Right->transform->SetRotation(-3.14 / 8.0, glm::vec3(0.0, 0.0, 1.0));

		/* Teapot */
		auto Teapot = E::Entity::Create("Teapot");
		Teapot->addComponent(CM::Meshes["Teapot"]);
		Teapot->addComponent(CM::Materials["RedTeapot"]);
		//Teapot->addComponent(CM::Materials["RedTeapot1"]);
		Teapot->addComponent(CM::Materials["ShadowMaterial"]);
		Teapot->transform->SetScale(.2, .2, .2);
		Teapot->transform->SetPosition(-4., 4., -4.);
		Teapot->transform->SetRotation(glm::radians(30.f), glm::vec3(0.0, 0.0, 1.0));
		Teapot->callbacks->update = [](std::shared_ptr<E::Entity> teapot) {
			//teapot->transform->RotateAround(glm::vec3(.3, -.25, -0.8), glm::vec3(0.0, 0.0, 1.0), -.25f);
		};

		/* Ball */
		auto Ball = E::Entity::Create("Ball");
		Ball->addComponent(CM::Meshes["Sphere"]);
		Ball->addComponent(CM::Materials["GreenBall"]);
		//Ball->addComponent(CM::Materials["GreenBall1"]);
		Ball->addComponent(CM::Materials["ShadowMaterial"]);
		Ball->transform->SetScale(2., 2., 2.);
		Ball->transform->SetPosition(3., 4., -7.5);

		/* Light */
		auto Light = E::Entity::Create("Light");
		Light->addComponent(CM::Lights["Light"]);
		Light->addComponent(CM::Meshes["Sphere"]);
		Light->addComponent(CM::Materials["LightMaterial"]);
		Light->addComponent(CM::Perspectives["P0"]);
		Light->transform->SetPosition(glm::vec3(0.0, 5., 5.));
		Light->transform->SetScale(glm::vec3(.5, .5, .5));
		Light->callbacks->update = [](std::shared_ptr<E::Entity> light) {
			light->transform->RotateAround(glm::vec3(0.0), glm::vec3(0.0, 0.0, 1.0), .5f);

			auto perspective = CM::Perspectives["P0"];
			/* Left */
			perspective->views[0] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(1.0, 0.0, 0.0),
				glm::vec3(0.0, 0.0, 1.0));

			/* Right */
			perspective->views[1] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(-1.0, 0.0, 0.0),
				glm::vec3(0.0, 0.0, 1.0));

			/* Top */
			perspective->views[2] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(0.0, 0.0, 1.0),
				glm::vec3(0.0, -1.0, 0.0));

			/* Bottom */
			perspective->views[3] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(0.0, 0.0, -1.0),
				glm::vec3(0.0, 1.0, 0.0));

			/* Front */
			perspective->views[4] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(0.0, 1.0, 0.0),
				glm::vec3(0.0, 0.0, 1.0));

			/* Back */
			perspective->views[5] = glm::lookAt(
				light->transform->GetPosition(),
				light->transform->GetPosition() + glm::vec3(0.0, -1.0, 0.0),
				glm::vec3(0.0, 0.0, 1.0));

			perspective->projections[0] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
			perspective->projections[1] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
			perspective->projections[2] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
			perspective->projections[3] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
			perspective->projections[4] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
			perspective->projections[5] = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
		};

		/* Skybox */
		auto Skybox = E::Entity::Create("Skybox");
		Skybox->addComponent(CM::Meshes["Sphere"]);
		Skybox->addComponent(CM::Materials["SkyboxMaterial"]);
		Skybox->transform->SetScale(100.0f, 100.0f, 100.0f);
	}

	void SetupSystems() {
		Systems::RenderSystem = []() {
			bool refreshRequired = false;
			auto lastTime = glfwGetTime();

			/* Take the perspective from the camera */
			auto P0 = CM::Perspectives["P0"];
			auto P1 = CM::Perspectives["P1"];

			/* Record the commands required to render the current scene. */
			P0->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
			P1->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));

			while (!VKDK::ShouldClose() && !Systems::quit) {
				auto currentTime = glfwGetTime();

				if (currentTime - lastTime > 1.0 / Systems::UpdateRate) {

					/* Update Entities */
					for (auto pair : Systems::SceneGraph::Entities) {
						if (pair.second->callbacks->update) {
							pair.second->callbacks->update(pair.second);
						}
					}

					/* Upload Transform UBOs */
					for (auto pair : Systems::SceneGraph::Entities) {
						glm::mat4 worldToLocal = pair.second->getWorldToLocalMatrix();
						glm::mat4 localToWorld = glm::inverse(worldToLocal);
						pair.second->transform->uploadUBO(worldToLocal, localToWorld);
					}

					/* Upload Material UBOs */
					for (auto pair : Systems::ComponentManager::Materials) {
						pair.second->material->uploadUBO();
					}

					/* Upload Perspective UBOs before render */
					for (auto pair : Systems::ComponentManager::Perspectives) {
						pair.second->uploadUBO();
					}

					/* Upload Point Light UBO */
					Components::Lights::PointLights::UploadUBO();

					/* Aquire a new image from the swapchain */
					refreshRequired |= VKDK::PrepareFrame();

					/* Submit offscreen pass to graphics queue */
					VKDK::SubmitToGraphicsQueueInfo offscreenPassInfo;
					offscreenPassInfo.graphicsQueue = VKDK::graphicsQueue;
					offscreenPassInfo.commandBuffers = {
					  P0->commandBuffers[0]
					};
					offscreenPassInfo.waitSemaphores = { VKDK::semaphores.presentComplete };
					offscreenPassInfo.signalSemaphores = { VKDK::semaphores.offscreenComplete };
					VKDK::SubmitToGraphicsQueue(offscreenPassInfo);


					/* Submit final pass to graphics queue  */
					VKDK::SubmitToGraphicsQueueInfo finalPassInfo;
					finalPassInfo.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
					finalPassInfo.graphicsQueue = VKDK::graphicsQueue;
					finalPassInfo.waitSemaphores = { VKDK::semaphores.offscreenComplete };
					finalPassInfo.signalSemaphores = { VKDK::semaphores.renderComplete };
					VKDK::SubmitToGraphicsQueue(finalPassInfo);

					/* Submit the frame for presenting. */
					refreshRequired |= VKDK::SubmitFrame();
					if (refreshRequired) {
						VKDK::RecreateSwapChain();

						/* Add a perspective to render the swapchain */
						P1->refresh(VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
							VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

						P1->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
						refreshRequired = false;
					}
					std::cout << "\r Framerate: " << currentTime - lastTime;
					lastTime = currentTime;
				}
			}
			vkDeviceWaitIdle(VKDK::device);
		};

		Systems::EventSystem = []() {
			while (!VKDK::ShouldClose() && !Systems::quit) {
				glfwPollEvents();
			}
		};

		Systems::UpdateSystem = []() {
			auto lastTime = glfwGetTime();
			while (!VKDK::ShouldClose() && !Systems::quit) {
				auto currentTime = glfwGetTime();
				if (currentTime - lastTime > 1.0 / Systems::UpdateRate) {



					lastTime = currentTime;
				}
			}
		};

		Systems::currentThreadType = Systems::SystemTypes::Event;
	}

	void CleanupComponents() {
		/* Cleanup components */
		CM::Cleanup();

		/* Destroy the requested material pipelines */
		Materials::UniformColor::Destroy();
		Materials::Blinn::Destroy();
		Materials::Skybox::Destroy();
		Materials::Shadow::Destroy();
	}
}

void StartDemo7() {
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 7 - Shadow Mapping", false, false, false, VK_API_VERSION_1_1 };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ7::SetupComponents();
	PRJ7::SetupEntities();
	PRJ7::SetupSystems();
	S::LaunchThreads();

	S::JoinThreads();
	PRJ7::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo7(); }
#endif