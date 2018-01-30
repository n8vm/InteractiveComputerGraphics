add_subdirectory(Model)
add_subdirectory(Cameras)

set (Entities_SRC 
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${CMAKE_CURRENT_SOURCE_DIR}/Entity.hpp
	${Model_SRC}
	${Cameras_SRC}
	PARENT_SCOPE)