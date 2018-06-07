// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm - Textures                                      |
// │   Example using a 2D texture to color a material. Photogrammetry |
// |   data from Nedo:                                                |
// |  https://sketchfab.com/models/fa37a3a07c2e4ff59e0f58fa9962bb48   |
// |  Note, RelWithDebug/Release build is recommended. Large OBJ file |
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

namespace PRJ4 {
	void SetupComponents() {
		CM::Initialize();

		auto perspective = Math::Perspective::Create("MainPerspective",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* Textures are loaded using the khronos texture format (.ktx)
			This format supports precomputed mipmapping, a couple compression formats, cubemaps, 3D textures, etc.
			To convert from a more common format, like a .png, I recommend using PVRTexTool by Imagination Graphics. */
		auto atticTexture = Textures::Texture2D::Create("Attic_Texture", ResourcePath "Dachboden/Dachboden_500K_u1_v1.ktx");

		/* Load meshes */
		Meshes::OBJMesh::Create("Attic", ResourcePath "Dachboden/Dachboden_500K.obj");
		Meshes::OBJMesh::Create("Teapot", ResourcePath "Teapot/teapot.obj");

		/* Initialize Materials */
		auto pipelineKey = PipelineKey(VKDK::renderPass, 0, 0);
		auto pipelineParameters = PipelineParameters::Create(pipelineKey);
		pipelineParameters->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		Materials::Blinn::Initialize(25);
		Materials::UniformColor::Initialize(25);

		glm::vec4 redAmbient = glm::vec4(0.05, 0.025, 0.025, 1.0);
		glm::vec4 red = glm::vec4(0.5, 0.0, 0.0, 1.0);

		auto texmat = Materials::UniformColor::Create("TextureMaterial", pipelineKey);
		auto redMat = Materials::Blinn::Create("RedMaterial", pipelineKey);
		auto whitemat = Materials::UniformColor::Create("WhiteMaterial", pipelineKey);

		/* Some materials will accept textures as parameters.

			Note: unlike with OpenGL, we can't just bind 0 to an unused texture slot. Instead, the component manager
				loads a couple default resources which can be bound by default and replaced later.
		*/
		texmat->useTexture(true);
		texmat->setTexture(atticTexture);
		redMat->setColor(red, Colors::white, redAmbient);
		whitemat->setColor(Colors::white);

		/* Create a light component for shading */
		Lights::PointLights::Create("Light1");
		Lights::PointLights::Create("Light2");
	}

	void SetupEntities() {
		/* Create an orbit camera to look at the model */
		auto camera = Cameras::SpinTableCamera::Create("Camera",
			glm::vec3(0.0, -20.0, 4.0), glm::vec3(0.0, 0.0, 6.0), glm::vec3(0.0, 0.0, 1.0));
		camera->addComponent(CM::Perspectives["MainPerspective"]);

		/* Create the attic */
		auto attic = E::Entity::Create("Attic");
		attic->transform->SetPosition(-1.5, 1.0, 0.0);
		attic->transform->SetRotation(-3.0, glm::vec3(0.0, 0.0, 1.0));
		attic->addComponent(CM::Materials["TextureMaterial"], CM::Meshes["Attic"]);

		/* Create teapot to sit in the attic */
		auto teapot = E::Entity::Create("Teapot");
		teapot->transform->SetPosition(-6.0, 1.0, 0.95);
		teapot->transform->SetScale(.05, .05, .05);
		teapot->addComponent(CM::Materials["RedMaterial"], CM::Meshes["Teapot"]);

		/* Add finally some point lights */
		auto light1 = E::Entity::Create("Light1");
		light1->transform->SetPosition(-18.0, -2.2, 8);
		light1->transform->SetScale(glm::vec3(0.1, 0.1, 0.1));
		light1->addComponent(CM::Lights["Light1"], CM::Meshes["Sphere"], CM::Materials["WhiteMaterial"]);

		auto light2 = E::Entity::Create("Light2");
		light2->transform->SetPosition(-12.0, 10.2, 8);
		light2->transform->SetScale(glm::vec3(0.1, 0.1, 0.1));
		light2->addComponent(CM::Lights["Light2"], CM::Meshes["Sphere"], CM::Materials["WhiteMaterial"]);
	}

	void SetupSystems() {
		S::RenderSystem = []() {
			bool refreshRequired = false;
			auto lastTime = glfwGetTime();

			auto perspective = CM::Perspectives["MainPerspective"];
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
		/* Cleanup components */
		CM::Cleanup();

		/* Destroy the requested material pipelines */
		Materials::Blinn::Destroy();
		Materials::UniformColor::Destroy();
	}
}

void StartDemo4() {
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 4 - Textures", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ4::SetupComponents();
	PRJ4::SetupEntities();
	PRJ4::SetupSystems();
	S::LaunchThreads();

	S::JoinThreads();
	PRJ4::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo4(); }
#endif