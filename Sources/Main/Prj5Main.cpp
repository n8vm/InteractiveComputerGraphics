// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm - Render-Passes                                 |
// │   Example which uses separate render passes to render to texture |
// └──────────────────────────────────────────────────────────────────┘

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

namespace PRJ5 {
	void SetupComponents() {
		CM::Initialize();

		auto teapot = Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");
		glm::vec3 centroid = teapot->mesh->getCentroid();

		/* Creating a perspective with this constructor will render the scene onto a texture with the same name.
			In this case, a set of four Texture2D's will be added to the component manager, each of a 512 by 512 resolution
		*/
		auto P1_1 = Math::Perspective::Create("P1_1", 512, 512);
		auto P1_2 = Math::Perspective::Create("P1_2", 512, 512);
		auto P1_3 = Math::Perspective::Create("P1_3", 512, 512);
		auto P1_4 = Math::Perspective::Create("P1_4", 512, 512);

		/* Setup perspectives for each texture */
		glm::mat4 view = glm::lookAt(glm::vec3(0.0, -30.0, 12.0), centroid, glm::vec3(0.0, 0.0, 1.0));
		glm::mat4 proj = glm::perspective(glm::radians(90.f), 1.f, 0.01f, 1000.0f);
		P1_1->views[0] = P1_2->views[0] = P1_3->views[0] = P1_4->views[0] = view;
		P1_1->projections[0] = P1_2->projections[0] = P1_3->projections[0] = P1_4->projections[0] = proj;

		/* Perspective for the final render pass  */
		auto P2_1 = Math::Perspective::Create("P2_1",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		auto P1_1Key = PipelineKey(P1_1->renderpass, 0, 0);
		auto P1_2Key = PipelineKey(P1_2->renderpass, 0, 0);
		auto P1_3Key = PipelineKey(P1_3->renderpass, 0, 0);
		auto P1_4Key = PipelineKey(P1_4->renderpass, 0, 0);
		auto P2_1Key = PipelineKey(P2_1->renderpass, 0, 0);

		/* Make the first texture a point list, and the second a list of lines. */
		auto P1_1Params = PipelineParameters::Create(P1_1Key);
		auto P1_2Params = PipelineParameters::Create(P1_2Key);
		auto P1_3Params = PipelineParameters::Create(P1_3Key);
		auto P1_4Params = PipelineParameters::Create(P1_4Key);
		auto P2_1Params = PipelineParameters::Create(P2_1Key);
		P1_1Params->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		P1_2Params->rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		P2_1Params->rasterizer.cullMode = VK_CULL_MODE_NONE;

		/* Add a light */
		Lights::PointLights::Create("Light");

		/* Initialize Materials */
		Materials::Blinn::Initialize(25);
		Materials::UniformColor::Initialize(25);

		auto p1mat = Materials::UniformColor::Create("P1Mat", P2_1Key);
		auto p2mat = Materials::UniformColor::Create("P2Mat", P2_1Key);
		auto p3mat = Materials::UniformColor::Create("P3Mat", P2_1Key);
		auto p4mat = Materials::UniformColor::Create("P4Mat", P2_1Key);

		p1mat->useTexture(true); p2mat->useTexture(true); p3mat->useTexture(true); p4mat->useTexture(true);
		p1mat->setTexture(CM::Textures["P1_1"]);
		p2mat->setTexture(CM::Textures["P1_2"]);
		p3mat->setTexture(CM::Textures["P1_3"]);
		p4mat->setTexture(CM::Textures["P1_4"]);

		Materials::UniformColor::Create("P1TeapotMat", P1_1Key);
		Materials::UniformColor::Create("P2TeapotMat", P1_2Key);
		Materials::UniformColor::Create("P3TeapotMat", P1_3Key);
		Materials::Blinn::Create("P4TeapotMat", P1_4Key);
	}

	void SetupEntities() {
		/* Create an orbit camera to look at the planes */
		auto camera = Cameras::SpinTableCamera::Create("Camera",
			glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		camera->addComponent(CM::Perspectives["P2_1"]);

		/* Plane Cameras */
		auto TLCamera = E::Entity::Create("TLCamera");
		TLCamera->addComponent(CM::Perspectives["P1_1"]);

		auto TRCamera = E::Entity::Create("TRCamera");
		TRCamera->addComponent(CM::Perspectives["P1_2"]);

		auto BLCamera = E::Entity::Create("BLCamera");
		BLCamera->addComponent(CM::Perspectives["P1_3"]);

		auto BRCamera = E::Entity::Create("BRCamera");
		BRCamera->addComponent(CM::Perspectives["P1_4"]);

		/* Create four planes, which will show the results of the first renderpass */
		auto TL = E::Entity::Create("TopLeft");
		auto TR = E::Entity::Create("TopRight");
		auto BL = E::Entity::Create("BottomLeft");
		auto BR = E::Entity::Create("BottomRight");

		TL->transform->SetPosition(glm::vec3(-1.1, 1.1, 0.0));
		TR->transform->SetPosition(glm::vec3(1.1, 1.1, 0.0));
		BL->transform->SetPosition(glm::vec3(-1.1, -1.1, 0.0));
		BR->transform->SetPosition(glm::vec3(1.1, -1.1, 0.0));

		/* Add material & mesh */
		TL->addComponent(CM::Materials["P1Mat"], CM::Meshes["Plane"]);
		TR->addComponent(CM::Materials["P2Mat"], CM::Meshes["Plane"]);
		BL->addComponent(CM::Materials["P3Mat"], CM::Meshes["Plane"]);
		BR->addComponent(CM::Materials["P4Mat"], CM::Meshes["Plane"]);

		TL->callbacks->update = [](std::shared_ptr<E::Entity> panel) {
			panel->transform->SetRotation(sinf(-glfwGetTime() + .1), glm::vec3(.5, .5, 0.0));	};
		TR->callbacks->update = [](std::shared_ptr<E::Entity> panel) {
			panel->transform->SetRotation(sinf(-glfwGetTime() + .4), glm::vec3(.5, -.5, 0.0));	};
		BL->callbacks->update = [](std::shared_ptr<E::Entity> panel) {
			panel->transform->SetRotation(sinf(glfwGetTime() + .2), glm::vec3(.5, -.5, 0.0));	};
		BR->callbacks->update = [](std::shared_ptr<E::Entity> panel) {
			panel->transform->SetRotation(sinf(glfwGetTime() + .3), glm::vec3(.5, .5, 0.0));	};

		/* Create a teapot */
		auto Teapot = E::Entity::Create("Teapot");
		Teapot->addComponent(CM::Meshes["Teapot"]);

		/* add a material for each render pass where the teapot will be rendered.*/
		Teapot->addComponent(CM::Materials["P1TeapotMat"], CM::Materials["P2TeapotMat"], CM::Materials["P3TeapotMat"], CM::Materials["P4TeapotMat"]);

		Teapot->callbacks->update = [](std::shared_ptr<E::Entity> teapot) {
			/* Move teapot */
			teapot->transform->AddRotation(glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)));

			/* Change material color */
			auto P1Mat = CM::Materials["P1TeapotMat"];
			auto P2Mat = CM::Materials["P2TeapotMat"];
			auto P3Mat = CM::Materials["P3TeapotMat"];
			auto P4Mat = CM::Materials["P4TeapotMat"];
			auto UC1 = std::dynamic_pointer_cast<Materials::UniformColor>(P1Mat->material);
			auto UC2 = std::dynamic_pointer_cast<Materials::UniformColor>(P2Mat->material);
			auto UC3 = std::dynamic_pointer_cast<Materials::UniformColor>(P3Mat->material);
			auto UC4 = std::dynamic_pointer_cast<Materials::Blinn>(P4Mat->material);
			glm::vec4 newColor = glm::vec4(Colors::hsvToRgb(glm::vec3(glfwGetTime() * .1, 1.0, 1.0)), 1.0);
			UC1->setColor(newColor);
			UC2->setColor(newColor);
			UC3->setColor(newColor);
			UC4->setColor(newColor, Colors::white, Colors::black);
		};

		//Create a light for the blinn teapot
		auto light = E::Entity::Create("Light");
		light->addComponent(CM::Lights["Light"]);
		light->transform->SetPosition(glm::vec3(0.0, -30.0, 8.0));
	}

	void SetupSystems() {
		S::RenderSystem = []() {
			bool refreshRequired = false;
			auto lastTime = glfwGetTime();

			/* Take the perspective from the camera */
			auto P1_1 = CM::Perspectives["P1_1"];
			auto P1_2 = CM::Perspectives["P1_2"];
			auto P1_3 = CM::Perspectives["P1_3"];
			auto P1_4 = CM::Perspectives["P1_4"];
			auto P2_1 = CM::Perspectives["P2_1"];

			/* Record the commands required to render the current scene. */
			P1_1->recordRenderPass(glm::vec4(1.0, 1.0, 1.0, 1.0));
			P1_2->recordRenderPass(glm::vec4(1.0, 1.0, 1.0, 1.0));
			P1_3->recordRenderPass(glm::vec4(1.0, 1.0, 1.0, 1.0));
			P1_4->recordRenderPass(glm::vec4(1.0, 1.0, 1.0, 1.0));
			P2_1->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));

			while (!VKDK::ShouldClose()) {
				auto currentTime = glfwGetTime();

				/* Upload Perspective UBOs before render */
				for (auto pair : CM::Perspectives) {
					pair.second->uploadUBO();
				}

				/* Aquire a new image from the swapchain */
				refreshRequired |= VKDK::PrepareFrame();

				/* Submit offscreen pass to graphics queue */
				VKDK::SubmitToGraphicsQueueInfo offscreenPassInfo;
				offscreenPassInfo.graphicsQueue = VKDK::graphicsQueue;
				offscreenPassInfo.commandBuffers = {
					P1_1->commandBuffers[0],
			P1_2->commandBuffers[0],
			P1_3->commandBuffers[0],
			P1_4->commandBuffers[0]
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
					P2_1->refresh(VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
						VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

					P2_1->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
					refreshRequired = false;
				}
				std::cout << "\r Framerate: " << currentTime - lastTime;
				lastTime = currentTime;
			}
			vkDeviceWaitIdle(VKDK::device);
		};

		S::EventSystem = []() {
			while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
				glfwPollEvents();
			}
		};

		S::UpdateSystem = []() {
			auto lastTime = glfwGetTime();
			while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
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

		S::currentThreadType = Systems::SystemTypes::Event;
	}

	void CleanupComponents() {
		/* Cleanup components */
		CM::Cleanup();

		/* Destroy the requested material pipelines */
		Materials::Blinn::Destroy();
		Materials::UniformColor::Destroy();
	}
}

void StartDemo5() {
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 5 - Render Passes", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ5::SetupComponents();
	PRJ5::SetupEntities();
	PRJ5::SetupSystems();
	S::LaunchThreads();

	S::JoinThreads();
	PRJ5::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo5(); }
#endif