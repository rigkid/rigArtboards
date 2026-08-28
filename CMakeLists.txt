file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

# Deps are loaded by RigKitPacks from pack.json order. Do not add_subdirectory
# here - nested adds race the pack loader and reuse the same binary dirs.

add_library(${PACK_NAME} STATIC
	src/rigArtboards.cpp
	src/rigArtboards.h
	src/SurfaceView.cpp
	src/SurfaceView.h
	src/SurfaceXform.h
	src/SurfaceSeam.h
	src/SurfaceGizmo.cpp
	src/SurfaceGizmo.h
	src/SurfaceNumeric.cpp
	src/SurfaceNumeric.h
	src/SelectionBounds.cpp
	src/SelectionBounds.h
	src/PathTools.cpp
	src/PathTools.h
	src/SurfaceChrome.cpp
	src/SurfaceChrome.h
	src/PathfinderPanel.cpp
	src/PathfinderPanel.h
	src/AlignPanel.cpp
	src/AlignPanel.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
foreach(_dep IN ITEMS
		rigPlotComponent
		rigVectorEditor
		rigPlotter
		rigImGui)
	if(TARGET ${_dep})
		target_link_libraries(${PACK_NAME} PUBLIC ${_dep})
	endif()
endforeach()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/tools/surface_smoke.cpp")
	add_executable(surface_smoke "${CMAKE_CURRENT_LIST_DIR}/tools/surface_smoke.cpp")
	target_link_libraries(surface_smoke PRIVATE ${PACK_NAME})
endif()

message(STATUS "Configured pack: ${PACK_NAME} (shared artboard surface)")
