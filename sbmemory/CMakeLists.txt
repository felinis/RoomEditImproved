add_library(
	sbmemory
	"internal/Array.h"
	"MemoryPool.h"
	"MemoryPool.cc"
)

set_property(TARGET sbmemory PROPERTY CXX_STANDARD 17)
