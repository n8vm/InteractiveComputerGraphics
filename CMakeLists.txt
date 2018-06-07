cmake_minimum_required (VERSION 3.10)
project(Interactive_Computer_Graphics)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(${PROJECT_SOURCE_DIR}/CMakeGenerateHierarchy.txt)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(SRC_PATH "Sources")

#------------------------------------------------------------
# Options go here
#------------------------------------------------------------

#------------------------------------------------------------
# Use C++ 17
#------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

#------------------------------------------------------------
# DEFINITIONS
#------------------------------------------------------------
add_definitions(-DResourcePath="${CMAKE_CURRENT_SOURCE_DIR}/Resources/")

# Force binaries to go to build folder, not subfolders
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

#------------------------------------------------------------
# External dependencies
#------------------------------------------------------------

# use GLFW
find_package(GLFW REQUIRED)
include_directories(SYSTEM ${GLFW_INCLUDE_DIR})

# use GLM
find_package(GLM REQUIRED)
include_directories(SYSTEM ${GLM_INCLUDE_DIR})

# use GLI
find_package(GLI REQUIRED)
include_directories(SYSTEM ${GLI_INCLUDE_DIR})

# use Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIR})

# use Python
find_package(PythonLibs REQUIRED)
include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})

# add libraries to a list for linking
set(LIBRARIES
    ${PYTHON_LIBRARIES}
    ${Vulkan_LIBRARY}
    ${GLFW_LIBRARIES}
    ${OPENVR_LIBRARY})

#------------------------------------------------------------
# Source Files/Shaders/Kernels (Uses a recursive cmake tree)
#------------------------------------------------------------
add_subdirectory(Sources)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Sources)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
#------------------------------------------------------------
# EXECUTABLE TARGETS
#------------------------------------------------------------
add_executable (PRJ1-Hello-World "${PRJ1_SRC}")
target_link_libraries (PRJ1-Hello-World ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ1_SRC}")

add_executable (PRJ2-Transformations "${PRJ2_SRC}")
target_link_libraries (PRJ2-Transformations ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ2_SRC}")

add_executable (PRJ3-Shading "${PRJ3_SRC}")
target_link_libraries (PRJ3-Shading ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ3_SRC}")

add_executable (PRJ4-Textures "${PRJ4_SRC}")
target_link_libraries (PRJ4-Textures ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ4_SRC}")

add_executable (PRJ5-Render-Passes "${PRJ5_SRC}")
target_link_libraries (PRJ5-Render-Passes ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ5_SRC}")

add_executable (PRJ6-Environment-Mapping "${PRJ6_SRC}")
target_link_libraries (PRJ6-Environment-Mapping ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ6_SRC}")

add_executable (PRJ7-Shadow-Mapping "${PRJ7_SRC}")
target_link_libraries (PRJ7-Shadow-Mapping ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ7_SRC}")

add_executable (PRJ8-Voxel-Cone-Tracing "${PRJ8_SRC}")
target_link_libraries (PRJ8-Voxel-Cone-Tracing ${LIBRARIES} VKDK ECS)
generate_folder_hierarchy("${PRJ8_SRC}")

#------------------------------------------------------------
# INSTALL TARGETS
#------------------------------------------------------------
#install(TARGETS PRJ1-Hello-World DESTINATION ./)

