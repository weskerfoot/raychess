static Vector3 calculateMove(int, int, int);

enum ChessPiece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
};

// Pawn move offsets (including the initial two-square move)
Vector2 pawnOffsets[] = {
    {0, 1},  // Single square forward
    {0, 2}   // Two squares forward (only available as the first move)
};

// Knight move offsets
Vector2 knightOffsets[] = {
    {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
    {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
};

// Bishop move offsets (unbounded)
Vector2 bishopOffsets[] = {
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
};

// Rook move offsets (unbounded)
Vector2 rookOffsets[] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
};

// Queen move offsets (combines rook and bishop moves, unbounded)
Vector2 queenOffsets[] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, // Rook moves
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1} // Bishop moves
};

// King move offsets
Vector2 kingOffsets[] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
};
