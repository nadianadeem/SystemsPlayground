
#include "World.h"

#include "WorldUtils.h"

#include <stdexcept>
#include <cassert>

World::World(int width, int height)
    : m_width(width)
    , m_height(height)
{
    m_tiles.reserve(m_width * m_height);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Tile tile;
            tile.xCoord = x;
            tile.yCoord = y;
            tile.state = TileState::Unloaded;
            m_tiles.push_back(tile);
        }
    }
}

void World::UpdateActiveTiles(int playerX, int playerY, int activeRadius, int loadRadius)
{
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {

            Tile& tile = GetTile(x, y);
            int dist = WorldUtils::TileDistance(x, y, playerX, playerY);

            if (dist <= activeRadius) {
                tile.state = TileState::Active;
            }
            else if (dist <= loadRadius) {
                tile.state = TileState::Loaded;
            }
            else {
                tile.state = TileState::Unloaded;
            }
        }
    }
}


int World::Index(int x, int y) const
{
    assert(IsValidCoordinate(x, y));
        
    return y * m_width + x;
}

Tile& World::GetTile(int x, int y)
{   
    return m_tiles[Index(x, y)];
}

const Tile& World::GetTile(int x, int y) const
{
    return m_tiles[Index(x, y)];
}

bool World::IsValidCoordinate(int x, int y) const
{
    if (x >= m_width || y >= m_height || x < 0 || y < 0)
    {
        throw std::out_of_range("Tile coordinates are out of world bounds.");
    }

	return true;
}
