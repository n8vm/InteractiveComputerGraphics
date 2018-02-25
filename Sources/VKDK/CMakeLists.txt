include(${PROJECT_SOURCE_DIR}/CMakeGenerateHierarchy.txt)
set(VKDK_SRC 
	${CMAKE_CURRENT_SOURCE_DIR}/objload.h
	${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader.h
	${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanInitializers.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/vkdk.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vkdk.hpp)

add_library(VKDK STATIC ${VKDK_SRC})
target_include_directories(VKDK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# IDE Folder Hierarchy Generation
generate_folder_hierarchy("${VKDK_SRC}")