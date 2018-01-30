add_subdirectory(HaloMaterials)

set (Materials_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Material.hpp
	${HaloMaterials_SRC}
	PARENT_SCOPE) 