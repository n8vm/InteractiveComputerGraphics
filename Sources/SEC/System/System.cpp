#include "System.hpp"

namespace System {
	bool quit = false;

	thread *UpdateThread;
	thread *RaycastThread;
	int UpdateRate = 90;
	int FrameRate = 90;

	//Entity World = Entity();
	//Entity Screen = Entity();

	//unordered_map<string, std::shared_ptr<Texture>> TextureList;
	//unordered_map<string, std::shared_ptr<Material>> MaterialList;
	//unordered_map<string, std::shared_ptr<obj::Model>> ModelList;

	//std::shared_ptr<Entity> Raycaster;
	//bool MouseDown[5]; // TODO: fix this kludge
};
