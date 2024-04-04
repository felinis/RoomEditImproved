add_library(
	sbgraphics

	# heap management
	"base/heap/DynamicHeap.cc"
	"base/heap/UploadHeap.cc"
	"base/HeapManager.cc"

	"base/engineFeatures.cc"
	"base/sbBaseRenderer.cc"

	"sbRasterRenderer.cc"
#	"sbRTRenderer.cc"
)

#use the ray tracing engine feature
#target_compile_definitions(sbgraphics PRIVATE RAY_TRACING)

target_include_directories(sbgraphics PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(sbgraphics dxgi dxguid d3d12)
set_property(TARGET sbgraphics PROPERTY CXX_STANDARD 17)
