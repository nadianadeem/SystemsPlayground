
#pragma once

#include <cstdlib>
#include <algorithm>

namespace WorldUtils 
{
	inline int TileDistance(int x0, int y0, int x1, int y1) 
	{ 
		return std::max(std::abs(x0 - x1), std::abs(y0 - y1));
	}
}