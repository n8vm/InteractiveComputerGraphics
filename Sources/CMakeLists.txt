add_subdirectory(SEC)
add_subdirectory(VKDK)

add_subdirectory(Options)

set(SHARED_SRC 
  ${Options_SRC}
)

set (SHARED_KRNL )

set (SHARED_SHDR )

# PRJ1
set(PRJ1_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj1Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# PRJ2
set(PRJ2_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj2Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# PRJ3
set(PRJ3_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj3Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# PRJ4
set(PRJ4_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj4Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# PRJ5
set(PRJ5_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj5Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# PRJ6
set(PRJ6_SRC 
  ${SHARED_SRC}
  ${SHARED_KRNL}
  ${SHARED_SHDR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Main/Prj6Main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  PARENT_SCOPE)

# Kernel Source Files
set(KERNEL_SRC ${SHARED_KRNL})

# Shader Source Files
set(SHADER_SRC ${SHARED_SHDR})

# # Copy list of OpenCL kernels to a file for opencl runtime
# string(REPLACE ";" "\n" opencl_sources "${KERNEL_SRC}")
# file(WRITE OpenCL/opencl_sources.txt ${opencl_sources})
# file(WRITE OpenCL/opencl_sources.txt ${opencl_sources})

# # Copy list of OpenGL shaders to file for opengl runtime
# string(REPLACE ";" "\n" opengl_sources "${SHADER_SRC}")
# file(WRITE OpenGL/opengl_sources.txt ${opengl_sources})
# file(WRITE OpenGL/opengl_sources.txt ${opengl_sources})
