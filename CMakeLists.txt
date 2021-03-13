cmake_minimum_required(VERSION 3.15)

# Set Project Name and Version
project(Copperplate VERSION 0.0.1)

# Set the C++ Standard
set(CMAKE_CXX_STANDARD 17)

# Make all Libraries and Executables output to the same folder
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Allow using Folders within the VS project
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add every subdirectory with a CMakeLists File
add_subdirectory(Copperplate)
add_subdirectory(thirdparty/glfw-3.3.2)
add_subdirectory(thirdparty/glad)
