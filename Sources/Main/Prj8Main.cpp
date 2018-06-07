#include "vkdk.hpp"
#include "glm/glm.hpp"
#include "ecs.hpp"
#include <math.h>

namespace E = Entities;
namespace C = Components;
namespace S = Systems;

namespace CM = Systems::ComponentManager;
namespace SG = Systems::SceneGraph;

namespace Math = Components::Math;
namespace Lights = Components::Lights;
namespace Materials = Components::Materials::Standard;
namespace VolMaterials = Components::Materials::Volume;
namespace Meshes = Components::Meshes;
namespace Cameras = Entities::Cameras;
namespace Textures = Components::Textures;

namespace PRJ8 {
	void SetupComponents() {
		CM::Initialize();
		int voxSize = 128;
		int shadowSize = 1024;

		/* Load meshes and textures */
		Meshes::OBJMesh::Create("CornellBox", ResourcePath "Cornell/Cube.obj");
		Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");
		Meshes::OBJMesh::Create("CubeFrame", ResourcePath "Defaults/CubeFrame.obj");

		Textures::Texture2D::Create("CornellTexture", ResourcePath "Cornell/texture.ktx");
		Textures::Texture3D::Create("VoxelizationTexture", voxSize, voxSize, voxSize, true);

		/* Final perspective */
		auto P2 = Math::Perspective::Create("P2",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* Voxilization perspective */
		auto P1 = Math::Perspective::Create("P1", voxSize, voxSize);
		P1->views[0] = glm::lookAt(glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));

		/* Shadow map perspective */
		auto P0 = Math::Perspective::Create("P0", shadowSize, shadowSize, true);

		/* Initialize Materials */
		PipelineKey P0_0_0 = PipelineKey(P0->renderpass, 0, 0);
		PipelineKey P1_0_0 = PipelineKey(P1->renderpass, 0, 0);
		PipelineKey P2_0_0 = PipelineKey(P2->renderpass, 0, 0);
		PipelineKey P2_0_1 = PipelineKey(P2->renderpass, 0, 1);
		PipelineKey P2_0_2 = PipelineKey(P2->renderpass, 0, 2);
		auto P0_0_0_Params = PipelineParameters::Create(P0_0_0);
		auto P1_0_0_Params = PipelineParameters::Create(P1_0_0);
		auto P2_0_0_Params = PipelineParameters::Create(P2_0_0);
		auto P2_0_1_Params = PipelineParameters::Create(P2_0_1);
		auto P2_0_2_Params = PipelineParameters::Create(P2_0_2);
		P1_0_0_Params->rasterizer.cullMode = VK_CULL_MODE_NONE;
		P1_0_0_Params->depthStencil.depthTestEnable = VK_FALSE;
		P2_0_2_Params->rasterizer.cullMode = VK_CULL_MODE_NONE;
		P2_0_2_Params->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

		Materials::UniformColor::Initialize(25);
		Materials::Blinn::Initialize(25);
		VolMaterials::Raycast::Initialize(25);
		VolMaterials::Voxelize::Initialize(25);
		Materials::Shadow::Initialize(10);
		Materials::Skybox::Initialize(10);

		auto skymat = Materials::Skybox::Create("SkyboxMaterial", P2_0_2);
		auto cornellmat = Materials::Blinn::Create("CornellMat", P2_0_0);
		auto redmat = Materials::Blinn::Create("RedTeapot", P2_0_0);
		auto greenmat = Materials::Blinn::Create("GreenBall", P2_0_0);
		auto whitemat = Materials::Blinn::Create("White", P2_0_0);
		auto lightmat = Materials::UniformColor::Create("LightMaterial", P2_0_0);
		auto shadowMat = Materials::Shadow::Create("ShadowMaterial", P0_0_0);
		auto cornellVoxel = VolMaterials::Voxelize::Create("CornellVox", P1_0_0, CM::Textures["VoxelizationTexture"], CM::Textures["CornellTexture"]);
		auto redVox = VolMaterials::Voxelize::Create("RedVox", P1_0_0, CM::Textures["VoxelizationTexture"], Colors::red, Colors::white);
		auto greenVox = VolMaterials::Voxelize::Create("GreenVox", P1_0_0, CM::Textures["VoxelizationTexture"], Colors::green, Colors::white);
		auto whiteVox = VolMaterials::Voxelize::Create("WhiteVox", P1_0_0, CM::Textures["VoxelizationTexture"], Colors::white, Colors::white);
		VolMaterials::Raycast::Create("RaycastVoxels", P2_0_1, CM::Textures["VoxelizationTexture"]);
		VolMaterials::Voxelize::Create("LightVox", P1_0_0, CM::Textures["VoxelizationTexture"], Colors::white, Colors::white);

		lightmat->setColor(Colors::white);

		cornellmat->setColor(Colors::white, Colors::black);
		cornellmat->useDiffuseTexture(true);
		cornellmat->setDiffuseTexture(CM::Textures["CornellTexture"]);
		cornellmat->useGlobalIllumination(true);
		cornellmat->setGlobalIlluminationVolume(CM::Textures["VoxelizationTexture"]);
		cornellmat->useShadowMapTexture(true);
		cornellmat->setShadowMapTexture(CM::Textures["P0"]);

		redmat->setColor(Colors::red + glm::vec4(0.0, .1, .1, 0.0));
		redmat->useShadowMapTexture(true);
		redmat->setShadowMapTexture(CM::Textures["P0"]);
		redmat->useGlobalIllumination(true);
		redmat->setGlobalIlluminationVolume(CM::Textures["VoxelizationTexture"]);
		redmat->ka = Colors::red * .001f;

		greenmat->setColor(Colors::green + glm::vec4(0.1, .0, .1, 0.0));
		greenmat->useShadowMapTexture(true);
		greenmat->setShadowMapTexture(CM::Textures["P0"]);
		greenmat->useGlobalIllumination(true);
		greenmat->setGlobalIlluminationVolume(CM::Textures["VoxelizationTexture"]);
		greenmat->ka = Colors::green * .001f;


		whitemat->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0));
		whitemat->useShadowMapTexture(true);
		whitemat->setShadowMapTexture(CM::Textures["P0"]);
		whitemat->useGlobalIllumination(true);
		whitemat->setGlobalIlluminationVolume(CM::Textures["VoxelizationTexture"]);
		whitemat->ka = glm::vec4(1.0, 1.0, 1.0, 1.0) * .005f;

		whiteVox->ka = glm::vec4(1.0, 1.0, 1.0, 1.0) * .005f;
		whiteVox->useShadowMapTexture(true);
		whiteVox->setShadowMapTexture(CM::Textures["P0"]);

		greenVox->ka = Colors::green * .001f;
		greenVox->useShadowMapTexture(true);
		greenVox->setShadowMapTexture(CM::Textures["P0"]);

		redVox->ka = Colors::red * .001f;
		redVox->useShadowMapTexture(true);
		redVox->setShadowMapTexture(CM::Textures["P0"]);

		cornellVoxel->useShadowMapTexture(true);
		cornellVoxel->setShadowMapTexture(CM::Textures["P0"]);

		skymat->useTexture(true);
		skymat->setTexture(CM::Textures["P0"]);

		/* Add a light which casts shadows */
		auto l = Components::Lights::PointLights::Create("Light", true);
		l->setFalloffType(Components::Lights::SQUARED);
		l->setIntensity(55.f);
		l->setColor(glm::vec4(1., 0.9, 0.9, 1.0));
		//l->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0));
		//l->setColor(glm::vec4(.878, 0.722, 0.42, 1.0));

		l->setFalloffType(Components::Lights::SQUARED);
		l->setIntensity(5.f);
		//l->setIntensity(0.f);
	}

	void SetupEntities() {
		/* Create an orbit camera to look at the model */
		auto camera = Cameras::SpinTableCamera::Create("Camera", glm::vec3(0.0, 50.0f, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), false, glm::radians(30.f));
		camera->addComponent(CM::Perspectives["P2"]);
		camera->setZoomAcceleration(.1);
		//camera->setFieldOfView(glm::radians(80.));

		/* Cornell Box */
		auto CornellBox = E::Entity::Create("CornellBox");
		CornellBox->addComponent(CM::Meshes["CornellBox"]);
		CornellBox->addComponent(CM::Materials["CornellMat"]);
		CornellBox->addComponent(CM::Materials["CornellVox"]);
		CornellBox->addComponent(CM::Materials["ShadowMaterial"]);
		CornellBox->transform->SetScale(9.5, 9.5, 9.5);
		CornellBox->transform->SetPosition(0.0, 0.0, 0.0);

		/* Left Box */
		auto LeftBox = E::Entity::Create("LeftBox");
		LeftBox->addComponent(CM::Meshes["Cube"]);
		LeftBox->addComponent(CM::Materials["White"]);
		LeftBox->addComponent(CM::Materials["WhiteVox"]);
		LeftBox->addComponent(CM::Materials["ShadowMaterial"]);
		LeftBox->transform->SetScale(3., 3., 6.);
		LeftBox->transform->SetPosition(3., -3., -4.);
		LeftBox->transform->SetRotation(3.14 / 8.0, glm::vec3(0.0, 0.0, 1.0));

		/* Right Box */
		auto Right = E::Entity::Create("Right");
		Right->addComponent(CM::Meshes["Cube"]);
		Right->addComponent(CM::Materials["White"]);
		Right->addComponent(CM::Materials["WhiteVox"]);
		Right->addComponent(CM::Materials["ShadowMaterial"]);
		Right->transform->SetScale(3., 3., 3.);
		Right->transform->SetPosition(-4., 4., -7.);
		Right->transform->SetRotation(-3.14 / 8.0, glm::vec3(0.0, 0.0, 1.0));

		/* Teapot */
		auto Teapot = E::Entity::Create("Teapot");
		Teapot->addComponent(CM::Meshes["Teapot"]);
		Teapot->addComponent(CM::Materials["RedTeapot"]);
		Teapot->addComponent(CM::Materials["RedVox"]);
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
		Ball->addComponent(CM::Materials["GreenVox"]);
		Ball->addComponent(CM::Materials["ShadowMaterial"]);
		Ball->transform->SetScale(2., 2., 2.);
		Ball->transform->SetPosition(3., 4., -7.5);


		/* Light */
		auto Light = E::Entity::Create("Light");
		Light->addComponent(CM::Lights["Light"]);
		Light->addComponent(CM::Meshes["Sphere"]);
		Light->addComponent(CM::Materials["LightMaterial"]);
		Light->addComponent(CM::Materials["LightVox"]);
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

		/* Voxelization */
		auto Voxelization = E::Entity::Create("Voxelization");
		Voxelization->addComponent(CM::Meshes["Cube"]);
		//Voxelization->addComponent(CM::Materials["RaycastVoxels"]);
		Voxelization->callbacks->update = [](std::shared_ptr<E::Entity> voxelization) {
			auto mat = CM::Materials["RaycastVoxels"];
			auto raycastMat = std::dynamic_pointer_cast<Components::Materials::Volume::Raycast>(mat->material);
			raycastMat->lod = 2.0;// +(sinf(glfwGetTime() * .4) * 2.f);
		};
		Voxelization->transform->SetScale(10., 10., 10.);

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
			auto P2 = CM::Perspectives["P2"];

			P1->preRenderPassCallback = [](VkCommandBuffer commandBuffer) {
				auto volume = CM::Textures["VoxelizationTexture"];
				auto texture = volume->texture;
				VkClearColorValue clearColorValue = {};
				clearColorValue.float32[0] = 0;
				clearColorValue.float32[1] = 0;
				clearColorValue.float32[2] = 0;
				clearColorValue.float32[3] = 0;

				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = texture->getColorMipLevels();
				subresourceRange.layerCount = 1;
				vkCmdClearColorImage(commandBuffer, texture->getColorImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColorValue, 1, &subresourceRange);
			};

			P2->preRenderPassCallback = [](VkCommandBuffer commandBuffer) {
				auto volume = CM::Textures["VoxelizationTexture"];
				auto texture = std::dynamic_pointer_cast<Textures::Texture3D>(volume->texture);
				texture->generateColorMipMap(commandBuffer);
			};

			/* Record the commands required to render the current scene. */
			P0->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
			P1->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
			P2->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));

			while (!VKDK::ShouldClose() && !Systems::quit) {
				auto currentTime = glfwGetTime();
				if (currentTime - lastTime > 1.0 / Systems::UpdateRate) {
					/* Upload Transform UBOs */
					for (auto pair : Systems::SceneGraph::Entities) {
						glm::mat4 worldToLocal = pair.second->getWorldToLocalMatrix();
						glm::mat4 localToWorld = glm::inverse(worldToLocal);
						pair.second->transform->uploadUBO(worldToLocal, localToWorld);
					}

					/* Upload Material UBOs */
					for (auto pair : CM::Materials) {
						pair.second->material->uploadUBO();
					}

					/* Upload Perspective UBOs before render */
					for (auto pair : CM::Perspectives) {
						pair.second->uploadUBO();
					}

					/* Upload Point Light UBO */
					Components::Lights::PointLights::UploadUBO();

					/* Aquire a new image from the swapchain */
					refreshRequired |= VKDK::PrepareFrame();

					/* Submit offscreen pass to graphics queue */
					VKDK::SubmitToGraphicsQueueInfo offscreenPassInfo;
					offscreenPassInfo.graphicsQueue = VKDK::graphicsQueue;
					offscreenPassInfo.commandBuffers = { P0->commandBuffers[0] , P1->commandBuffers[0] };
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
						P2->refresh(VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
							VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

						P2->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
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
					/* Update Entities */
					for (auto pair : Systems::SceneGraph::Entities) {
						if (pair.second->callbacks->update) {
							pair.second->callbacks->update(pair.second);
						}
					}

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
		VolMaterials::Raycast::Destroy();
		VolMaterials::Voxelize::Destroy();
		Materials::Shadow::Destroy();
		Materials::Skybox::Destroy();
	}
}

void StartDemo8() {
	VKDK::InitializationParameters vkdkParams = { 512, 512, "Project 8 - Voxel Cone Tracing", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ8::SetupComponents();
	PRJ8::SetupEntities();
	PRJ8::SetupSystems();
	S::LaunchThreads();

	S::JoinThreads();
	PRJ8::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo8(); }
#endif