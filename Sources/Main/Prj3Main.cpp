// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm - Shading                                       |
// │   Shows how lights can be used to illuminate certain materials,  |
// |   like Blinn Phong.                                              |
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

namespace PRJ3 {
	void SetupComponents() {
		/* Initialize component manager */
		CM::Initialize();

		/* Setup perspectives */
		auto perspective = Math::Perspective::Create("MainPerspective",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* Initialize Pipeline Settings */
		auto pipelineKey = PipelineKey(VKDK::renderPass, 0, 0);
		auto pipelineParameters = PipelineParameters::Create(pipelineKey);

		/* Initialize Materials */
		Materials::UniformColor::Initialize(3);
		Materials::Blinn::Initialize(4);

		/* Create Material Instances */
		auto redmat = Materials::Blinn::Create("RedMaterial", pipelineKey);
		auto greenmat = Materials::Blinn::Create("GreenMaterial", pipelineKey);
		auto bluemat = Materials::Blinn::Create("BlueMaterial", pipelineKey);
		auto greymat = Materials::Blinn::Create("GreyMaterial", pipelineKey);
		auto whitemat = Materials::UniformColor::Create("WhiteMaterial", pipelineKey);

		redmat->setColor(Colors::red);
		greenmat->setColor(Colors::green);
		bluemat->setColor(Colors::blue);
		greymat->setColor(Colors::darkGrey);
		whitemat->setColor(Colors::white);

		/* Create a light component for shading */
		auto l1 = Lights::PointLights::Create("Light1");
		auto l2 = Lights::PointLights::Create("Light2");
		auto l3 = Lights::PointLights::Create("Light3");

		l1->setFalloffType(Lights::FalloffType::NONE);
		l2->setFalloffType(Lights::FalloffType::LINEAR);
		l3->setFalloffType(Lights::FalloffType::SQUARED);

		/* Load meshes */
		Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");
	}

	void SetupEntities() {
		auto centriod = CM::Meshes["Teapot"]->mesh->getCentroid();
		centriod *= .1f;
		auto camera = Cameras::SpinTableCamera::Create("Camera",
			glm::vec3(0.0f, -7.0f, 5.0f), glm::vec3(0.0f, 0.0f, centriod.z), glm::vec3(0.0f, 0.0f, 1.0f));
		camera->addComponent(CM::Perspectives["MainPerspective"]);

		/* Create a set of teapots */
		auto redTeapot = E::Entity::Create("RedTeapot");
		redTeapot->transform->SetPosition(-1.5f, 1.0f, 0.0f);
		redTeapot->transform->SetScale(.1f, .1f, .1f);
		redTeapot->transform->SetRotation(-3.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		redTeapot->addComponent(CM::Materials["RedMaterial"], CM::Meshes["Teapot"]);

		auto greenTeapot = E::Entity::Create("GreenTeapot");
		greenTeapot->transform->SetPosition(0.0f, -1.5f, 0.0f);
		greenTeapot->transform->SetScale(.1f, .1f, .1f);
		greenTeapot->transform->SetRotation(-2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		greenTeapot->addComponent(CM::Materials["GreenMaterial"], CM::Meshes["Teapot"]);

		auto blueTeapot = E::Entity::Create("BlueTeapot");
		blueTeapot->transform->SetPosition(1.75f, 0.5f, 0.0f);
		blueTeapot->transform->SetScale(.1f, .1f, .1f);
		blueTeapot->transform->SetRotation(-0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
		blueTeapot->addComponent(CM::Materials["BlueMaterial"], CM::Meshes["Teapot"]);

		/* ... and a floor to set the teapots on */
		auto floor = E::Entity::Create("Floor");
		floor->transform->SetScale(4.0f, 4.0f, 4.0f);
		floor->addComponent(CM::Meshes["Plane"], CM::Materials["GreyMaterial"]);

		/* Add finally some point lights*/
		auto light1 = E::Entity::Create("Light1");
		light1->transform->SetPosition(glm::vec3(0.0f, 3.0f, 1.5f));
		light1->transform->SetScale(glm::vec3(0.1f, 0.1f, 0.1f));
		light1->addComponent(CM::Materials["WhiteMaterial"], CM::Meshes["Sphere"], CM::Lights["Light1"]);
		light1->callbacks->update = [](std::shared_ptr<E::Entity> light) {
			light->transform->RotateAround(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0.1f);
			light->transform->AddPosition(glm::vec3(0.0f, 0.0f, 0.015f) * sinf((float)glfwGetTime()));
		};

		auto light2 = E::Entity::Create("Light2");
		light2->transform->SetPosition(glm::vec3(-3.0f * sqrtf(3.0f) * .5f, 3.0f * -.5f, 1.5f));
		light2->transform->SetScale(glm::vec3(0.1f, 0.1f, 0.1f));
		light2->addComponent(CM::Materials["WhiteMaterial"], CM::Meshes["Sphere"], CM::Lights["Light2"]);
		light2->callbacks->update = [](std::shared_ptr<E::Entity> light) {
			light->transform->RotateAround(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), .1f);
			light->transform->AddPosition(glm::vec3(0.0f, 0.0f, 0.015f) * sinf((float)glfwGetTime()));
		};

		auto light3 = E::Entity::Create("Light3");
		light3->transform->SetPosition(glm::vec3(3.0 * sqrtf(3.0f) * .5f, 3.0f * -.5f, 1.5f));
		light3->transform->SetScale(glm::vec3(0.1, 0.1, 0.1));
		light3->addComponent(CM::Materials["WhiteMaterial"], CM::Meshes["Sphere"], CM::Lights["Light3"]);
		light3->callbacks->update = [](std::shared_ptr<E::Entity> light) {
			light->transform->RotateAround(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), .1f);
			light->transform->AddPosition(glm::vec3(0.0f, 0.0f, 0.015f) * sinf((float)glfwGetTime()));
		};
	}

	void SetupSystems() {
		S::RenderSystem = []() {
			bool refreshRequired = false;
			auto lastTime = glfwGetTime();

			/* Take the perspective from the camera */
			auto perspective = CM::Perspectives["MainPerspective"];

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
					for (auto pair : CM::Lights) {
						Lights::PointLights::UploadUBO();
					}

					lastTime = currentTime;
				}
			}
		};

		S::currentThreadType = S::SystemTypes::Event;
	}

	void CleanupComponents() {
		CM::Cleanup();

		/* Destroy the requested material pipelines */
		Materials::UniformColor::Destroy();
		Materials::Blinn::Destroy();
	}
}

void StartDemo3() {
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 3 - Shading", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ3::SetupComponents();
	PRJ3::SetupEntities();
	PRJ3::SetupSystems();
	S::LaunchThreads();

	S::JoinThreads();
	PRJ3::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo3(); }
#endif