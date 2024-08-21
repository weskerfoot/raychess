static Vector3 calculateMove(int, int, int);

enum PLAYER_TYPES {
  WHITE_PLAYER = 0,
  BLACK_PLAYER = 1
};

enum SIDES {
  TOP_SIDE = 0,
  BOTTOM_SIDE = 1
};

enum CONTROL_AXES {
  LEFT_STICK_LEFT_RIGHT = 0,
  LEFT_STICK_UP_DOWN = 1,
  RIGHT_STICK_LEFT_RIGHT = 2,
  RIGHT_STICK_UP_DOWN = 3,
  LEFT_TRIGGER = 4,
  RIGHT_TRIGGER = 5
};

enum ChessPiece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
};

struct Cells {
  Vector3 *grid_positions;
  Vector2 *chess_positions;
};

struct ChessPieces {
  int *chess_type;
  char *is_dead;
  struct Cells cells;
};

struct ChessTypes {
  Texture2D *textures;
  Model *models;
  float *scaling_factors;
  Vector2 **offsets; // "actions" a piece type can take
  int *offset_sizes;
};

struct Players {
  int *score;
  int *player_type;
  struct ChessPieces *pieces;
};

// TODO have multiple boards

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
