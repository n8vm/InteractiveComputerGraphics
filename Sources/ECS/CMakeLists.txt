# ECS - System Entity Component
include(${PROJECT_SOURCE_DIR}/CMakeGenerateHierarchy.txt)
add_subdirectory(Components)
add_subdirectory(Entities)
add_subdirectory(Systems)
add_subdirectory(Tools)

set(ECS_SRC 
	${CMAKE_CURRENT_SOURCE_DIR}/ecs.hpp
	${Components_SRC}
	${Entities_SRC}
	${Systems_SRC}
	${Tools_SRC})

add_library(ECS STATIC
	${ECS_SRC})

target_include_directories(ECS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ECS PUBLIC VKDK)

# IDE Folder Hierarchy Generation
generate_folder_hierarchy("${ECS_SRC}")