
#include "pch.h"

#include "../../../Engine/src/World/World.h"

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