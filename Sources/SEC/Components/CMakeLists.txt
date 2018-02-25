add_subdirectory(Textures)
add_subdirectory(Materials)
add_subdirectory(Meshes)

set (Components_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Transform.hpp
	${Materials_SRC}
	${Meshes_SRC}
	${Textures_SRC}
	PARENT_SCOPE) 
