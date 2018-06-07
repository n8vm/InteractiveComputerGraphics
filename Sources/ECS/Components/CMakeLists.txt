add_subdirectory(Textures)
add_subdirectory(Materials)
add_subdirectory(Math)
add_subdirectory(Meshes)
add_subdirectory(Callbacks)
add_subdirectory(Lights)

set (Components_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Component.hpp
	${Materials_SRC}
	${Math_SRC}
	${Meshes_SRC}
	${Callbacks_SRC}
	${Textures_SRC}
	${Lights_SRC}
	PARENT_SCOPE) 
