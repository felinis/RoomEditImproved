#pragma once

#ifdef RAY_TRACING
//use the ray-tracing renderer
#include "sbRTRenderer.hh"
#define sbRenderer sbRTRenderer
#else
//use the raster renderer
#include "sbRasterRenderer.hh"
#define sbRenderer sbRasterRenderer
#endif
