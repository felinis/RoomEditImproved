add_executable(
	roomedit
	WIN32
	"FreeLookCamera.cc"
	"GameWorld.cc"
	"world_renderer.cc"
	"main.cc"
	"scene_view.cc"
)
target_include_directories(roomedit PUBLIC ${CMAKE_SOURCE_DIR}) #treat the root dir as an include dir
target_link_libraries(roomedit sbinput sbfilesystem sbmemory sbgraphics comctl32)
set_property(TARGET roomedit PROPERTY CXX_STANDARD 17)
