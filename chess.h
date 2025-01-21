static Vector3 calculate_position(int, int, int);
static int clamp(int, int, int);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define PIECE_SIZE 5.0f
#define N_ROWS 8
#define N_COLS 8
#define N_CELLS (N_ROWS*N_COLS)
#define N_PIECES 16
#define GRID_SIZE (N_CELLS  * 5.0f) // TODO would be stored in a table later, and the tile size 5.0 would be dynamic

int
next_pow2(int n) {
  int k = 1;
  while (k < n) {
    k *= 2;
  }
  return k;
}

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
  NUM_PLAYERS
} PlayerType;

typedef enum ChessPiece {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
} ChessPiece;

// There will be one object mapped to one quad
// When setting pieces/objects on the grid we do the quadtree insertion
//  which involves drilling down to the largest quad that can contain only that object
// When moving a piece, we set the piece index for that quad to -1 to indicate it's now empty
// then we do the quadtree insertion in the new place
//
// if there is a conflict we would have to make a smaller quad, or we could not move there
// to start with we will assume we can't move there unless we're killing / destroying the object in the new spot
//
// quadtree insertion algorithm:
//  keep going until you find the smallest existing quad that can fit the object
//    check if the bounding box of your object is fully contained by the bounding box of the quad, if the object would be split into two quads we have to stop
//    if the quad contains another object then we either split or if we've reached the limit of divisions, that should not happen (would be an error or we destroyed/killed the existing object)
//
//
//    the initial insertion should be done in set_pieces
//    later it would be baked into the level data (or not? the level data could contain the size of the tree maybe)

struct Quads {
  int size;
  Vector3 *quad_positions;
  Vector2 *quad_sizes;
  int *piece_indices; // refers to pieces inside a box
  int *top_left;
  int *top_right;
  int *bottom_left;
  int *bottom_right;
};

struct ChessPieces {
  ChessPiece *chess_type;
  Vector3 *grid_positions; // where they are in 3D space
  Vector2 *chess_positions; // where they are in 2D chess space (centered around 0,0)
  uint8_t *is_dead;
  Color *colors;
  int *action_points_per_turn;
  int *piece_cell_indices; // foreign key for Cells
  int *quad_indices; // refers to the node in the quadtree this piece is located
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
  int *select_to_move_pieces; // tracks which cell you / a piece is actually on
  int *select_to_move_to_cells; // tracks which cell you're thinking of moving to
  int *live_piece_counts; // how many pieces are currently alive
  Vector2 *select_to_move_to_chess_positions; // tracks the chess position of the cell you're thinking of moving to
  PlayerType *player_type;
  PlayerState *player_states;
  int *piece_indices; // FIXME shouldn't be pointers, should be indices
};

struct Cells {
  uint8_t *occupied_states;
  int *cell_player_states;
  int *cell_piece_indices; // foreign key for ChessPieces
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

// Debugging stuff

// Convert HSV to RGB
Color HSVtoRGB(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }

    Color color = {
        (unsigned char)((r + m) * 255),
        (unsigned char)((g + m) * 255),
        (unsigned char)((b + m) * 255),
        255 // Fully opaque
    };

    return color;
}

// Function to map integer `i` to a distinct color
Color next_color(int i) {
    const float golden_ratio_conjugate = 0.618033988749895f; // Golden ratio conjugate
    float hue = fmodf((i * golden_ratio_conjugate) * 360.0f, 360.0f); // Spread hues evenly
    return HSVtoRGB(hue, 1.0f, 1.0f); // Full saturation and brightness
}

static void
print_vec2(Vector2 vec) {
  printf("x = %f, y = %f\n", vec.x, vec.y);
}

static void
print_vec3(Vector3 vec) {
  printf("x = %f, y = %f, z = %f\n", vec.x, vec.y, vec.z);
}

static void
print_cell_player_states(int *states) {
  printf("cell states = ");
  for (int i = 0; i < N_CELLS; i++) {
    printf("%d", states[i]);
  }
  printf("\n");
}


static void
print_board_state(uint8_t *board) {
  printf("board states = ");
  for (int i = 0; i < N_CELLS; i++) {
    printf("%d", board[i]);
  }
  printf("\n");
}

struct QItem {
  Vector3 position;
  Vector2 dimensions;
};

static int q_head = 0; // Index of the first element in the queue
static int q_tail = 0; // Index of the next insertion point
static int q_count = 0; // Number of elements in the queue

static int
q_push(struct QItem vec,
       struct QItem *queue,
       int q_size) {
    if (q_count == q_size) {
        return -1; // Queue is full
    }
    int index = q_tail;
    queue[q_tail] = vec;
    q_tail = (q_tail + 1) % q_size;
    q_count++;
    return index;
}

static int
q_get(int q_size) {
    if (q_count == 0) {
        return -1; // Queue is empty
    }
    int index = q_head;
    q_head = (q_head + 1) % q_size;
    q_count--;
    return index;
}
