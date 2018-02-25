#include "System.hpp"

namespace System {
	using namespace std;
	atomic<bool> quit = false;

	thread *UpdateThread;
	thread *EventThread;
	thread *RenderThread;
	thread *RaycastThread;
	int UpdateRate = 90;
	int FrameRate = 90;
	
	std::shared_ptr<Scene> MainScene;
	unordered_map<string, std::shared_ptr<Components::Textures::Texture>> TextureList;
	unordered_map<string, shared_ptr<Components::Meshes::Mesh>> MeshList;
};
