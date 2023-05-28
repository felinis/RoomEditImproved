#pragma once

#ifdef RAY_TRACING
//use the ray-tracing renderer
#include "sbRTRenderer.h"
#define sbRenderer sbRTRenderer
#else
//use the raster renderer
#include "sbRasterRenderer.h"
#define sbRenderer sbRasterRenderer
#endif
