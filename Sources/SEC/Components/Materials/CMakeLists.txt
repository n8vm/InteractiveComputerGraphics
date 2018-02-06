add_subdirectory(HaloMaterials)
add_subdirectory(SurfaceMaterials)

set (Materials_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Material.hpp
	${HaloMaterials_SRC}
	${SurfaceMaterials_SRC}
	PARENT_SCOPE) 