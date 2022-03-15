# List of all the source files, add new files here
set(SOURCE_LIST
	main.cpp
	core.h core.cpp
	application.h application.cpp
	scene.h scene.cpp
	mesh.h	mesh.cpp
	shader.h shader.cpp
	rendering.h rendering.cpp
	hatching.h hatching.cpp
	hatchingline.h hatchingline.cpp
	hatchinglayer.h hatchinglayer.cpp
	image.h image.cpp
	utility.h utility.cpp
	statistics.h statistics.cpp
	stb_image_write.h
	stb_image.h
	shaders/flatcolor.vert
	shaders/flatcolor.frag
	shaders/contours.vert
	shaders/contours.frag
	shaders/extractcontours.vert
	shaders/extractcontours.geom
	shaders/displaytex.vert
	shaders/displaytex.frag
	shaders/displaytexalpha.vert
	shaders/displaytexalpha.frag
	shaders/normals.vert
	shaders/normals.frag
	shaders/curvature.vert
	shaders/curvature.frag
	shaders/transformseeds.comp
	shaders/screenpoints.vert
	shaders/screenpoints.frag
	shaders/depth.vert
	shaders/depth.frag
	shaders/hatchinglines.vert
	shaders/hatchinglines.frag
	shaders/movement.vert
	shaders/movement.frag
	shaders/spherenormals.vert
	shaders/spherenormals.frag
	shaders/diffuse.vert
	shaders/diffuse.frag
	shaders/shadingGradient.vert
	shaders/shadingGradient.frag
	shaders/hatching.vert
	shaders/hatching.geom
	shaders/hatching.frag
)

# Setup as an executable
add_executable(Copperplate ${SOURCE_LIST})

# Make the folder structure within the VS project follow the folder structure of the files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})

# Link the libraries
target_link_libraries(Copperplate glfw)
target_link_libraries(Copperplate glad)
target_link_libraries(Copperplate glm)
target_link_libraries(Copperplate assimp)

# Copy the shader files to output after each build
add_custom_command(
	TARGET Copperplate POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/shaders
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)
