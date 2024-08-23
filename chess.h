static Vector3 calculateMove(int, int, int);

#define N_ROWS 8
#define N_COLS 8
#define N_CELLS (N_ROWS*N_COLS)
#define N_PIECES (N_ROWS*2)

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

typedef enum PlayerState {
  PIECE_SELECTION = 0,
  PIECE_MOVE = 1,
  CHECKMATE = 2
} PlayerState;

// FIXME allow for a variable number of players (for not chess)
typedef enum PlayerType {
  WHITE_PLAYER = 0,
  BLACK_PLAYER = 1
} PlayerType;

typedef enum ChessPiece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
} ChessPiece;

struct Cells {
  Vector3 *grid_positions;
  Vector2 *chess_positions;
};

struct ChessPieces {
  ChessPiece *chess_type;
  char *is_dead;
  struct Cells cells;
};

struct ChessTypes {
  float *scaling_factors;
  int *offset_sizes;
  Texture2D *textures;
  Model *models;
  Vector2 **offsets; // "actions" a piece type can take
};

struct Players {
  int *score;
  int *select_to_move_cells; // tracks which cell you / a piece is actually on
  int *select_to_move_to_cells; // tracks which cell you're thinking of moving to
  int *possible_cell_select_counts; // how many cells they could select currently
  PlayerType *player_type;
  PlayerState *player_states;
  struct ChessPieces *pieces;
};

// TODO have multiple boards

// Pawn move offsets (including the initial two-square move)
Vector2 pawnOffsets[] = {
    {1, 0},  // Single square forward
    {2, 0}   // Two squares forward (only available as the first move)
};

// Knight move offsets
Vector2 knightOffsets[] = {
    {1, 2}, {-1, 2}, {1, -2}, {-1, -2},
    {2, 1}, {-2, 1}, {2, -1}, {-2, -1}
};

// Bishop move offsets (unbounded)
Vector2 bishopOffsets[] = {
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
};

// Rook move offsets (unbounded)
Vector2 rookOffsets[] = {
    {0, 1}, {0, -1}, {1, 0}, {-1, 0}
};

// Queen move offsets (combines rook and bishop moves, unbounded)
Vector2 queenOffsets[] = {
    {0, 1}, {0, -1}, {1, 0}, {-1, 0}, // Rook moves
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1} // Bishop moves
};

// King move offsets
Vector2 kingOffsets[] = {
    {0, 1}, {0, -1}, {1, 0}, {-1, 0},
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
};
