add_subdirectory(Materials)

set (Components_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Transform.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Mesh.hpp
	${Materials_SRC}
	PARENT_SCOPE) 
