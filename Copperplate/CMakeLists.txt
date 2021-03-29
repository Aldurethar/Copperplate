# List of all the source files, add new files here
set(SOURCE_LIST
	main.cpp
	application.h application.cpp
	mesh.h	mesh.cpp
	shader.h shader.cpp
	rendering.h rendering.cpp
	shaders/flatcolor.vert
	shaders/flatcolor.frag
)

# Setup as an executable
add_executable(Copperplate ${SOURCE_LIST})

# Make the folder structure within the VS project follow the folder structure of the files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})

# Link the libraries
target_link_libraries(Copperplate glfw)
target_link_libraries(Copperplate glad)
target_link_libraries(Copperplate glm)

# Copy the shader files to output after each build
add_custom_command(
	TARGET Copperplate POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/shaders
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)
