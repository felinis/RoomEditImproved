add_library(
	sbinput
	"InputCollector.cc"
	"InputCollector.h"
)

target_link_libraries(sbinput dxguid dinput8)
set_property(TARGET sbinput PROPERTY CXX_STANDARD 17)
