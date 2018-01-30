# SEC - System Entity Component
include(${PROJECT_SOURCE_DIR}/CMakeGenerateHierarchy.txt)
add_subdirectory(System)
add_subdirectory(Entities)
add_subdirectory(Components)

set(SEC_SRC 
	${System_SRC}
	${Entities_SRC}
	${Components_SRC})

add_library(SEC STATIC
	${SEC_SRC})

target_include_directories(SEC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SEC PUBLIC VKDK)

# IDE Folder Hierarchy Generation
generate_folder_hierarchy("${SEC_SRC}")