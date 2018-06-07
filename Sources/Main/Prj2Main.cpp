// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm - Transformations                               |
// │   Shows how to create a camera, load a mesh, setup a material,   |
// |     to ultimately render a rotating RGB teapot                   |
// └──────────────────────────────────────────────────────────────────┘

#include "vkdk.hpp"
#include "glm/glm.hpp"
#include "ecs.hpp"

/* Namespaces can get a bit out of hand. We can create shortcuts like this. */
namespace ComponentManager = Systems::ComponentManager;
namespace SceneGraph = Systems::SceneGraph;
namespace Math = Components::Math;
namespace Materials = Components::Materials::Standard;
namespace Meshes = Components::Meshes;
namespace Cameras = Entities::Cameras;

namespace PRJ2 {
	void SetupComponents() {
		ComponentManager::Initialize();

		auto perspective = Math::Perspective::Create("My Perspective 1",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* In this implementation, pipelines have cooresponding "pipeline keys" and "pipeline parameters".
			The pipelne key is used to associate a renderpass, a subpass, and what I call a "pipeline index" to a pipeline.

			Given a pipeline key, we can create a PipelineParameters struct. These parameters are the graphics pipeline parameters
			which will be used when we build a pipeline. In this case, we'll tell the input assember to use a triangle list, and we'll
			tell the rasterizer to render only the edges of the triangles, and to disable back face culling.

			For different renderpasses/subpasses/pipeline indexes, we may have different pipeline properties.
		*/
		auto pipelineKey = PipelineKey(VKDK::renderPass, 0, 0);
		auto pipelineParameters = PipelineParameters::Create(pipelineKey);
		pipelineParameters->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineParameters->rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		pipelineParameters->rasterizer.cullMode = VK_CULL_MODE_NONE;

		/* We must request a maximum number of a particular material component that will be used by initializing the material with a count.
			Under the hood, this is allocating a Vulkan descriptor pool.
		*/
		Materials::UniformColor::Initialize(1);

		/* Create a material component by passing in the pipeline key.
			Under the hood this is building a graphics pipeline with the associated pipeline parameters component.
		*/
		Materials::UniformColor::Create("My Material 1", pipelineKey);

		/* Create a teapot mesh
			Under the hood, this allocates and uploads per vertex mesh data to Vulkan buffers
		*/
		Meshes::OBJMesh::Create("My Mesh 1", ResourcePath "Teapot/teapot.obj");
	}

	void SetupEntities() {
		/* Create a camera to look at our scene, and add a perspective component to it.
			A camera can be created from scratch by adding a perspective component to any entity, and adding a custom update callback.
			Cameras can be thought of as a "prefab".
		*/
		auto centroid = ComponentManager::Meshes["My Mesh 1"]->mesh->getCentroid();
		auto camera = Cameras::SpinTableCamera::Create("Camera", glm::vec3(0.0, -35.0, 35.0), glm::vec3(0.0, 0.0, centroid.z), glm::vec3(0.0, 0.0, 1.0));
		SceneGraph::Entities["Camera"]->addComponent(ComponentManager::Perspectives["My Perspective 1"]);

		/* Create a teapot, adding a material and a mesh as components */
		auto Teapot = Entities::Entity::Create("Teapot");

		/* We can add multiple components simultaneously */
		Teapot->addComponent(ComponentManager::Materials["My Material 1"], ComponentManager::Meshes["My Mesh 1"]);

		/* Tell the teapot to rotate using a lambda callback */
		Teapot->callbacks->update = [](std::shared_ptr<Entities::Entity> teapot) {
			/* All entities have a transform component by default. */
			teapot->transform->AddRotation(glm::angleAxis(0.01f, glm::vec3(0.0, 0.0, 1.0)));

			/* Change material color */
			auto uniformColor = ComponentManager::GetMaterial<Materials::UniformColor>("My Material 1");
			auto newColor = glm::vec4(Colors::hsvToRgb(glm::vec3(glfwGetTime() * .1, 1.0, 1.0)), 1.0);
			uniformColor->setColor(newColor);
		};
	}

	void SetupSystems() {
		Systems::RenderSystem = []() {
			bool refreshRequired = false;
			auto lastTime = glfwGetTime();

			/* Take the perspective from the camera */
			auto perspective = ComponentManager::Perspectives["My Perspective 1"];

			/* Pre-record the commands required to render the current scene.
				Note that although we've recorded 'how' to render the scene, the data from our components
				like material values or transform data can still change from one frame to the next.

				Under the hood, values pointed to by descriptor sets may change.
			*/
			perspective->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));

			while (!VKDK::ShouldClose() && !Systems::quit) {
				auto currentTime = glfwGetTime();

				/* Upload Perspective UBOs before render
					(Todo: implement circular buffering to handle race conditions) */
				for (auto pair : ComponentManager::Perspectives) {
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

					/* Record the new final render pass */
					perspective->recordRenderPass(glm::vec4(0.0, 0.0, 0.0, 0.0));
					refreshRequired = false;
				}
				std::cout << "\r Framerate: " << currentTime - lastTime;
				lastTime = currentTime;
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
					for (auto pair : SceneGraph::Entities) {
						if (pair.second->callbacks->update) {
							pair.second->callbacks->update(pair.second);
						}
					}

					/* Upload Transform UBOs */
					for (auto pair : SceneGraph::Entities) {
						auto worldToLocal = pair.second->getWorldToLocalMatrix();
						auto localToWorld = glm::inverse(worldToLocal);
						pair.second->transform->uploadUBO(worldToLocal, localToWorld);
					}

					/* Upload Material UBOs */
					for (auto pair : ComponentManager::Materials) {
						pair.second->material->uploadUBO();
					}
					lastTime = currentTime;
				}
			}
		};

		Systems::currentThreadType = Systems::SystemTypes::Event;
	}

	void CleanupComponents() {
		/* Cleanup components */
		ComponentManager::Cleanup();

		/* Destroy the requested material pipelines */
		Materials::UniformColor::Destroy();
	}
}

void StartDemo2() {
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 2 - Transformations", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	PRJ2::SetupComponents();
	PRJ2::SetupEntities();
	PRJ2::SetupSystems();
	Systems::LaunchThreads();

	Systems::JoinThreads();
	PRJ2::CleanupComponents();
	VKDK::Terminate();
}

#ifndef NO_MAIN
int main() { StartDemo2(); }
#endif