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
# include(CheckCXXCompilerFlag)
# CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
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

#------------------------------------------------------------
# External dependencies
#------------------------------------------------------------

# use GLFW
find_package(GLFW REQUIRED)
include_directories(SYSTEM ${GLFW_INCLUDE_DIR})

# use GLM
find_package(GLM REQUIRED)
include_directories(SYSTEM ${GLM_INCLUDE_DIR})

# use Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIR})

# add libraries to a list for linking
set(LIBRARIES
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
target_link_libraries (PRJ1-Hello-World ${LIBRARIES} VKDK SEC)
generate_folder_hierarchy("${PRJ1_SRC}")

add_executable (PRJ2-Transformations "${PRJ2_SRC}")
target_link_libraries (PRJ2-Transformations ${LIBRARIES} VKDK SEC)
generate_folder_hierarchy("${PRJ2_SRC}")

#------------------------------------------------------------
# INSTALL TARGETS
#------------------------------------------------------------
#install(TARGETS PRJ1-Hello-World DESTINATION ./)

