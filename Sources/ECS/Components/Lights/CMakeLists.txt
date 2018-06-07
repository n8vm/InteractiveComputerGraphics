add_subdirectory(PointLight)

set(Lights_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/Light.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Lights.hpp
	${PointLight_SRC}
	PARENT_SCOPE)