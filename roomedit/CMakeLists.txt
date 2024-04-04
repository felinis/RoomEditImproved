add_executable(
	roomedit
	WIN32

	#program-specific shading pipelines
	"shaders/ColoredSurfaceGraphicsPipeline.cc"
	"shaders/LightMappedSurfaceGraphicsPipeline.cc"
	"shaders/PhongSurfaceGraphicsPipeline.cc"

	#UI elements
	"ui/ButtonsPanel.cc"
	"ui/PropertyGrid.cc"
	"ui/StatusBar.cc"
	"ui/TreeView.cc"

	#world data
	"world/Actor.cc"
	"world/ActorWAD.cc"
	"world/common.cc"
	"world/Emitter.cc"
	"world/Light.cc"
	"world/Mesh.cc"
	"world/Room.cc"
	"world/SpotEffect.cc"
	"world/TextureInformation.cc"

	"BBox.cc"
	"Document.cc"
	"FreeLookCamera.cc"
	"GameWorld.cc"
	"main.cc"
	#OrbitCamera.cc not used for now
	"SceneView.cc"
	"WorldRenderer.cc"
)
target_include_directories(roomedit PUBLIC ${CMAKE_SOURCE_DIR}) #treat the root dir as an include dir
target_link_libraries(roomedit sbfilesystem sbmemory sbgraphics comctl32)
