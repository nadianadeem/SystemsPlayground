
#include "pch.h"

#include "../../../Engine/src/World/World.h"
#include "../../../Engine/src/World/WorldUtils.h"

TEST(World, TileRetrievalAndCoordinates) 
{
	const int width = 10;
	const int height = 10;
	World world(width, height);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			Tile& tile = world.GetTile(x, y);
			EXPECT_EQ(tile.xCoord, x);
			EXPECT_EQ(tile.yCoord, y);
			EXPECT_EQ(tile.state, TileState::Unloaded);
		}
	}
}

TEST(World, InvalidTileCoordinatesThrow) 
{
	const int width = 5;
	const int height = 5;
	World world(width, height);
	EXPECT_THROW(world.GetTile(-1, 0), std::out_of_range);
	EXPECT_THROW(world.GetTile(0, -1), std::out_of_range);
	EXPECT_THROW(world.GetTile(width, 0), std::out_of_range);
	EXPECT_THROW(world.GetTile(0, height), std::out_of_range);
	EXPECT_THROW(world.GetTile(width + 1, height + 1), std::out_of_range);
}

TEST(World, UpdateActiveTilesChangesTileStates) 
{
	const int width = 10;
	const int height = 10;
	World world(width, height);
	const int playerX = 5;
	const int playerY = 5;
	const int activeRadius = 2;
	const int loadRadius = 4;
	world.UpdateActiveTiles(playerX, playerY, activeRadius, loadRadius);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			Tile& tile = world.GetTile(x, y);
			int dist = WorldUtils::TileDistance(x, y, playerX, playerY);
			if (dist <= activeRadius) {
				EXPECT_EQ(tile.state, TileState::Active);
			}
			else if (dist <= loadRadius) {
				EXPECT_EQ(tile.state, TileState::Loaded);
			}
			else {
				EXPECT_EQ(tile.state, TileState::Unloaded);
			}
		}
	}
}