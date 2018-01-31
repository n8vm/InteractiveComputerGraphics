#include "System.hpp"

namespace System {
	using namespace std;
	bool quit = false;

	thread *UpdateThread;
	thread *RaycastThread;
	int UpdateRate = 90;
	int FrameRate = 90;

	int currentImageIndex = 0;

	Entities::Entity World = Entities::Entity();
	//Entity Screen = Entity();

	//unordered_map<string, std::shared_ptr<Texture>> TextureList;
	//unordered_map<string, shared_ptr<Components::Materials::Material>> MaterialList;
	unordered_map<string, shared_ptr<Components::Mesh>> MeshList;

	//std::shared_ptr<Entity> Raycaster;
	//bool MouseDown[5]; // TODO: fix this kludge
};
