add_library(
	sbmemory
	"Array.hh"
	"MemoryPool.hh"
	"MemoryPool.cc"
)

set_property(TARGET sbmemory PROPERTY CXX_STANDARD 17)
