add_subdirectory(Standard)
add_subdirectory(Volume)

set (Materials_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Material.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Materials.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/PipelineParameters.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/MaterialProperties.hpp
	${Standard_SRC}
	${Volume_SRC}
	PARENT_SCOPE) 