#pragma once

enum class TileState {
    Unloaded,   // Not in memory
    Loading,    // Being prepared (async later)
    Loaded,     // Data is ready
    Active      // In use / visible / simulated
};

struct Tile {
    int xCoord;
    int yCoord;
    TileState state;

    // TODO: for data used to generate mesh
    // std::unique_ptr<Heightmap> heightmap;
    // std::unique_ptr<Mesh> mesh;
    // std::vector<Object> objects;
};
