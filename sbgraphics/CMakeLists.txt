add_library(
	sbgraphics
	"base/engineFeatures.cc"
	"base/engineFeatures.h"
	"base/sbBaseRenderer.cc"
	"base/sbBaseRenderer.h"
	"sbRasterRenderer.cc"
	"sbRasterRenderer.h"
	"sbRenderer.h"
#	"sbRTRenderer.cc"
#	"sbRTRenderer.h"
)

#use the ray tracing engine feature
#target_compile_definitions(sbgraphics PRIVATE RAY_TRACING)

target_include_directories(sbgraphics PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(sbgraphics dxgi dxguid d3d12)
set_property(TARGET sbgraphics PROPERTY CXX_STANDARD 17)
