static Vector3 calculateMove(int, int, int);

#define N_ROWS 8
#define N_COLS 8
#define N_CELLS (N_ROWS*N_COLS)
#define N_PIECES (N_ROWS*2)

enum SIDES {
  TOP_SIDE = 0,
  BOTTOM_SIDE = 1
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

struct ChessPieces {
  ChessPiece *chess_type;
  Vector3 *grid_positions; // where they are in 3D space
  Vector2 *chess_positions; // where they are in 2D chess space (centered around 0,0)
  char *is_dead;
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
  Vector2 *select_to_move_to_chess_positions; // tracks the chess position of the cell you're thinking of moving to
  int *possible_move_counts; // how many cells they could select currently
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


Vector2 queenOffsets[] = {
    {0, -1},  // Vertical move in the leftmost column
    {-1, -1}, {1, -1}, // Diagonals in the leftmost columns (closer to the edge)
    {-1, 0}, {1, 0},  // Horizontal moves (rows)
    {-1, 1}, {1, 1},  // Diagonals in the rightmost columns (closer to the edge)
    {0, 1}   // Vertical move in the rightmost column
};

Vector2 kingOffsets[] = {
    {0, -1},           // Vertical move in the leftmost column
    {-1, -1}, {1, -1}, // Diagonals in the leftmost columns (closer to the edge)
    {-1, 0}, {1, 0},   // Horizontal moves (rows)
    {-1, 1}, {1, 1},   // Diagonals in the rightmost columns (closer to the edge)
    {0, 1}             // Vertical move in the rightmost column
};

