
#pragma once

#include "Tile.h"

#include <vector>

class World {
public:
    World(int width, int height);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    Tile& GetTile(int x, int y);
    const Tile& GetTile(int x, int y) const;

	bool IsValidCoordinate(int x, int y) const;
private:

    int m_width;
    int m_height;
    std::vector<Tile> m_tiles;

    int Index(int x, int y) const;
};
