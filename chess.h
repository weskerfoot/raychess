static Vector3 calculateMove(int, int, int);

#define N_ROWS 8
#define N_COLS 8
#define N_CELLS (N_ROWS*N_COLS)
#define N_PIECES (N_ROWS*2)

enum SIDES {
  TOP_SIDE = 0,
  BOTTOM_SIDE = 1
};

typedef enum CollisionStates {
  NO_COLLISION = 0,
  OWN_PIECE = 1,
  OTHER_PIECE = 2,
  BOARD_EDGE = 3
} CollisionStates;

typedef enum PlayerState {
  PIECE_SELECTION = 0,
  PIECE_MOVE = 1,
  CHECKMATE = 2
} PlayerState;

// FIXME allow for a variable number of players (for not chess)
typedef enum PlayerType {
  WHITE_PLAYER = 0,
  BLACK_PLAYER = 1,
} PlayerType;

typedef enum ChessPiece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
} ChessPiece;

struct ChessPieces {
  ChessPiece *chess_type;
  Vector3 *grid_positions; // where they are in 3D space
  Vector2 *chess_positions; // where they are in 2D chess space (centered around 0,0)
  uint8_t *is_dead;
  Color *colors;
  PlayerType *player_type;
  int *action_points_per_turn;
};

struct ChessTypes {
  float *scaling_factors;
  Texture2D *textures;
  Model *models;
  Vector2 **offsets; // "actions" a piece type can take
  int *offset_sizes;
};

struct Players {
  int *score;
  int *select_to_move_cells; // tracks which cell you / a piece is actually on
  int *select_to_move_to_cells; // tracks which cell you're thinking of moving to
  int *live_piece_counts;
  int *possible_move_counts; // how many cells they could select currently
  Vector2 *select_to_move_to_chess_positions; // tracks the chess position of the cell you're thinking of moving to
  PlayerType *player_type;
  PlayerState *player_states;
  struct ChessPieces *pieces; // FIXME shouldn't be pointers, should be indices
};

struct Cells {
  uint8_t *occupied_states;
  int *cell_player_states;
  int *cell_piece_indices; // index to the piece currently occupying a cell
};

// TODO have multiple boards

// Pawn move offsets (including the initial two-square move)
Vector2 pawnOffsets[] = {
    {1, 0},  // Single square forward
             // FIXME, need a rule that it can take other pieces diagonally only
};

// Knight move offsets
Vector2 knightOffsets[] = {
    {1, -2}, {-1, -2}, // Leftmost columns
    {2, -1}, {-2, -1}, // Next from left
    {2, 1}, {-2, 1},   // Next from left
    {1, 2}, {-1, 2}    // Rightmost columns
};

// Bishop move offsets (unbounded)
Vector2 bishopOffsets[] = {
    {1, -1}, {-1, -1}, // Leftmost columns
    {1, 1}, {-1, 1}    // Rightmost columns
};

// Rook move offsets (unbounded)
Vector2 rookOffsets[] = {
    {1, 0}, {-1, 0}, // Horizontal moves (rows)
    {0, -1}, {0, 1}  // Vertical moves (columns)
};

// Unbounded
Vector2 queenOffsets[] = {
    {0, -1},
    {-1, -1}, {1, -1},
    {-1, 0}, {1, 0},
    {-1, 1}, {1, 1},
    {0, 1}
};

Vector2 kingOffsets[] = {
    {0, -1},
    {-1, -1}, {1, -1},
    {-1, 0}, {1, 0},
    {-1, 1}, {1, 1},
    {0, 1}
};

