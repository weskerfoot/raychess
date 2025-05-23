#include "stdint.h"
#include "raylib.h"
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "chess.h"
#include "camera/rlTPCamera.h"

const int NINTENDO_CONTROLLER = 1;

static int
left_x_right_control() {
  int gamepad_x = GetGamepadAxisMovement(NINTENDO_CONTROLLER, GAMEPAD_AXIS_LEFT_X) > 0.95f;
  int key_x = IsKeyDown(KEY_D);
  return gamepad_x || key_x;
}

static int
left_x_left_control() {
  int gamepad_x = GetGamepadAxisMovement(NINTENDO_CONTROLLER, GAMEPAD_AXIS_LEFT_X) < 0;
  int key_x = IsKeyDown(KEY_A);
  return gamepad_x || key_x;
}

static int
left_y_down_control() {
  int gamepad_x = GetGamepadAxisMovement(NINTENDO_CONTROLLER, GAMEPAD_AXIS_LEFT_Y) > 0.95f;
  int key_x = IsKeyDown(KEY_S);
  return gamepad_x || key_x;
}

static int
left_y_up_control() {
  int gamepad_x = GetGamepadAxisMovement(NINTENDO_CONTROLLER, GAMEPAD_AXIS_LEFT_Y) < 0;
  int key_x = IsKeyDown(KEY_W);
  return gamepad_x || key_x;
}

static int
trigger_control() {
  int gamepad_trigger = IsGamepadButtonDown(NINTENDO_CONTROLLER, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
  int key_trigger = IsKeyDown(KEY_X);
  return gamepad_trigger || key_trigger;
}

static int
select_control() {
  int gamepad_control = IsGamepadButtonDown(NINTENDO_CONTROLLER, GAMEPAD_BUTTON_RIGHT_FACE_UP);
  int key_control = IsKeyDown(KEY_M);
  return gamepad_control || key_control;
}

static int
switch_players_control() {
  int gamepad_control = IsGamepadButtonDown(NINTENDO_CONTROLLER, GAMEPAD_BUTTON_LEFT_FACE_UP);
  int key_control = IsKeyDown(KEY_P);
  return gamepad_control || key_control;
}

// Piece stuff
static ChessPiece white_starting_pieces[N_PIECES] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,           // First row
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK // Second row
};

static ChessPiece black_starting_pieces[N_PIECES] = {
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK, // Second row
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN           // First row
};

static Color black_colors[N_PIECES] = {
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

static Color white_colors[N_PIECES] = {
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

static int white_starting_aps[N_PIECES] = {
    1, 1, 1, 1, 1, 1, 1, 1,     // First row
    N_COLS, 1, N_COLS, N_COLS, 1, N_COLS, 1, N_COLS, // Second row
};

static int black_starting_aps[N_PIECES] = {
    N_COLS, 1, N_COLS, N_COLS, 1, N_COLS, 1, N_COLS, // Second row
    1, 1, 1, 1, 1, 1, 1, 1     // First row
};

static int white_piece_cell_indices[N_PIECES];
static int black_piece_cell_indices[N_PIECES];

static Texture2D piece_textures[6];
static Model piece_models[6];
static float piece_scaling_factors[6] = {20.0f, 20.0f, 20.0f, 20.0f, 20.0f, 20.0f};

static int offset_sizes[6] = {
  (sizeof pawnOffsets)/sizeof(pawnOffsets[0]),
  (sizeof knightOffsets)/sizeof(knightOffsets[0]),
  (sizeof bishopOffsets)/sizeof(bishopOffsets[0]),
  (sizeof rookOffsets)/sizeof(rookOffsets[0]),
  (sizeof queenOffsets)/sizeof(queenOffsets[0]),
  (sizeof kingOffsets)/sizeof(kingOffsets[0])
};

static Vector2 *offsets[6] = {
  &pawnOffsets[0],
  &knightOffsets[0],
  &bishopOffsets[0],
  &rookOffsets[0],
  &queenOffsets[0],
  &kingOffsets[0]
};

static Vector3 white_grid_positions[N_PIECES];
static Vector3 black_grid_positions[N_PIECES];
static Vector2 white_chess_positions[N_PIECES];
static Vector2 black_chess_positions[N_PIECES];
static uint8_t white_pieces_dead[N_PIECES];
static uint8_t black_pieces_dead[N_PIECES];

// Cell stuff
static uint8_t occupied_states[N_CELLS];
static int cell_player_states[N_CELLS];
static int cell_piece_indices[N_CELLS];

static void
load_assets() {
    piece_models[PAWN] = LoadModel("resources/models/chess_pieces_models/pawn.glb");
    piece_models[KNIGHT] = LoadModel("resources/models/chess_pieces_models/knight.glb");
    piece_models[BISHOP] = LoadModel("resources/models/chess_pieces_models/bishop.glb");
    piece_models[ROOK] = LoadModel("resources/models/chess_pieces_models/rook.glb");
    piece_models[QUEEN] = LoadModel("resources/models/chess_pieces_models/queen.glb");
    piece_models[KING] = LoadModel("resources/models/chess_pieces_models/king.glb");
    return;
}

static int
convert_coord(int input, int n) {
  // n = number of cells in a row or column
  // Translate coordinates
  assert (n > 0);
  int half = n / 2;
  return half - input;
}

void
initialize_qtree(struct Quads qtree, struct QItem *queue, int q_size) {
  qtree.size = q_size;

  printf("qtree.size = %d\n", qtree.size);

  Vector2 current_quad_size;
  current_quad_size.x = N_ROWS * PIECE_SIZE;
  current_quad_size.y = N_COLS * PIECE_SIZE;

  Vector3 current_quad_position;
  current_quad_position.x = 0.0f;
  current_quad_position.y = 0.0f;
  current_quad_position.z = 0.0f;

  struct QItem root = {.position = current_quad_position, .dimensions=current_quad_size};

  q_push(root, queue, q_size);

  while (q_count > 0) { // FIXME level calculation
    int next_q_count = q_count;
    printf("q_count = %d\n", q_count);
    if (next_q_count >= (q_size/16.0)) {
      break;
    }
    for (int i = 0; i < next_q_count; i++) {
      struct QItem current_node = queue[q_get(q_size)];
      struct Vector2 root_dimensions = current_node.dimensions;
      struct Vector3 root_position = current_node.position;

      int root_num_cells_x = (root_dimensions.x / PIECE_SIZE);
      int root_num_cells_y = (root_dimensions.y / PIECE_SIZE);

      float root_x_numerator_left = root_dimensions.x;
      float root_x_numerator_right = root_dimensions.x;
      float root_y_numerator = root_dimensions.y;

      printf("i = %d, root_x_cells = %d, root_y_cells = %d\n", i, root_num_cells_x, root_num_cells_y);

      // Calculate root dimension number of cells
      // Split into quads based on number of *cells*, and allow an uneven split, e.g. 3 -> 1, 2
      // do that for rows and columns
      // convert back to normal coordinates for each quad

      Vector3 bottom_right_pos = {
          .x = root_position.x + (root_x_numerator_right / 4.0),
          .y = 0.0f,
          .z = root_position.z + (root_y_numerator / 4.0)
      };
      Vector3 bottom_left_pos = {
          .x = root_position.x - (root_x_numerator_left / 4.0),
          .y = 0.0f,
          .z = root_position.z + (root_y_numerator / 4.0)
      };
      Vector3 top_left_pos = {
          .x = root_position.x - (root_x_numerator_left / 4.0),
          .y = 0.0f,
          .z = root_position.z - (root_y_numerator / 4.0)
      };
      Vector3 top_right_pos = {
          .x = root_position.x + (root_x_numerator_right / 4.0),
          .y = 0.0f,
          .z = root_position.z - (root_y_numerator / 4.0)
      };

      Vector2 bottom_right_size = {.x=root_dimensions.x / 2.0, .y=root_dimensions.y / 2.0};
      Vector2 bottom_left_size = {.x=root_dimensions.x / 2.0, .y=root_dimensions.y / 2.0};
      Vector2 top_left_size = {.x=root_dimensions.x / 2.0, .y=root_dimensions.y / 2.0};
      Vector2 top_right_size = {.x=root_dimensions.x / 2.0, .y=root_dimensions.y / 2.0};

      //DrawCube(top_left_pos, top_left_size.x, 0.1f, top_left_size.y, next_color(i));
      //DrawCube(top_right_pos, top_right_size.x, 0.1f, top_right_size.y, next_color(i+10));
      //DrawCube(bottom_left_pos, bottom_left_size.x, 0.1f, bottom_left_size.y, next_color(i+20));
      //DrawCube(bottom_right_pos, bottom_right_size.x, 0.1f, bottom_right_size.y, next_color(i+30));

      assert(q_push((struct QItem){.position=top_left_pos, .dimensions=top_left_size}, queue, q_size) != -1);
      assert(q_push((struct QItem){.position=top_right_pos, .dimensions=top_right_size}, queue, q_size) != -1);
      assert(q_push((struct QItem){.position=bottom_right_pos, .dimensions=bottom_right_size}, queue, q_size) != -1);
      assert(q_push((struct QItem){.position=bottom_left_pos, .dimensions=bottom_left_size}, queue, q_size) != -1);

      /*
      printf("===============\n");
      printf("root_pos = "); print_vec3(root_position);
      printf("bottom_right_pos = "); print_vec3(bottom_right_pos);
      printf("top_right_pos = "); print_vec3(top_right_pos);
      printf("bottom_left_pos = "); print_vec3(bottom_left_pos);
      printf("top_left_pos = "); print_vec3(top_left_pos);
      printf("===============\n");
      */
    }
  }

  q_count = 0;
  q_tail = 0;
  q_head = 0;

}

static Vector3
calculate_position(int col, int row, int size) {
  // Given a column and row, and a tile size
  // calculate a board position
  assert (size != 0);
  Vector3 position = { (size*row) - (size/2.0),
                       0.0f,
                       (size*col) - (size/2.0)}; // 4 = half the grid from center
  assert (fpclassify(position.x) == FP_NORMAL || fpclassify(position.x) == FP_ZERO);
  assert (fpclassify(position.y) == FP_NORMAL || fpclassify(position.y) == FP_ZERO);
  assert (fpclassify(position.z) == FP_NORMAL || fpclassify(position.z) == FP_ZERO);

  return position;
}

static struct ChessPieces
set_pieces(struct ChessPieces pieces,
           struct Cells cells,
           struct Players players,
           int size,
           unsigned int side,
           int player_id) {
  int cell_id = 0;
  int start;
  int end;

  if (side == BOTTOM_SIDE) {
    start = 0;
    end = N_PIECES;
  }

  else {
    start = N_CELLS - (N_PIECES);
    end = N_CELLS;
  }

  int x_half = (N_ROWS / 2.0) - 1;
  int y_half = (N_COLS / 2.0);

  for (float i = -x_half; i <= y_half; i++) {
    for (float j = -x_half; j <= y_half; j++) {
      Vector3 position = calculate_position(i, j, size);
      // there are always 16 pieces per player
      if (cell_id >= start && cell_id < end) {
        pieces.grid_positions[cell_id % N_PIECES] = position;
        pieces.chess_positions[cell_id % N_PIECES] = (Vector2){i, j};
        pieces.is_dead[cell_id % N_PIECES] = 0;
        pieces.piece_cell_indices[cell_id % N_PIECES] = cell_id; // points to the cell that piece is on

        cells.occupied_states[N_CELLS - cell_id - 1] = 1;
        cells.cell_player_states[N_CELLS - cell_id - 1] = player_id;
        cells.cell_piece_indices[N_CELLS - cell_id - 1] = cell_id % N_PIECES; // ends up pointing back to the piece occupied by that cell
        players.select_to_move_pieces[player_id] = cell_id % N_PIECES;

        assert(pieces.grid_positions[cells.cell_piece_indices[N_CELLS - cell_id - 1]].y == position.y);
      }
      cell_id++;
    }
  }
  assert (cell_id < (N_ROWS*N_COLS) + 1);
  return pieces;
}

static int
should_skip_cell(int x,
                 int y,
                 int player_sign,
                 int active_player,
                 struct Cells cells) {
  // Filter out moves off the end of the board
  int position = y + (x * N_COLS);
  if (x < 0 || y < 0 || x >= N_ROWS || y >= N_COLS) {
    return BOARD_EDGE;
  }

  // If it's occupied but not by us then we can move to it (and take the piece on it in chess)

  if (cells.cell_player_states[position] == active_player) {
    return OWN_PIECE;
  }

  if (cells.cell_player_states[position] != active_player && cells.occupied_states[position] != 0) {
    return OTHER_PIECE;
  }

  return NO_COLLISION;
}

static int
handle_moving_piece(int piece_size,
                    int active_cell_to_move_to,
                    int active_piece_to_move,
                    int active_player,
                    int player_sign,
                    struct ChessPieces active_pieces,
                    struct Players active_players,
                    Vector2 active_chess_pos,
                    struct ChessTypes chess_types,
                    struct Cells cells) {

  if (active_pieces.is_dead[active_piece_to_move] == 1) {
    return 0;
  }

  int active_piece_type = active_pieces.chess_type[active_piece_to_move];
  int active_piece_aps = active_pieces.action_points_per_turn[active_piece_to_move];

  assert(active_piece_aps > 0);

  Vector2 *offsets = chess_types.offsets[active_piece_type];
  int offsetNum = chess_types.offset_sizes[active_piece_type];
  Vector2 move_chess_pos;

  int move_to_count = 0;

  for (int offsetIndex = 0; offsetIndex < offsetNum; offsetIndex++) {
    Vector2 offset = offsets[offsetIndex];
    offset.x = offset.x * player_sign;
    offset.y = offset.y * player_sign;

    // Used to refer to the active chess position in x/y coordinates
    int origin_x = convert_coord(active_chess_pos.x, N_ROWS);
    int origin_y = convert_coord(active_chess_pos.y, N_COLS);

    int scaled_x = convert_coord(active_chess_pos.x, N_ROWS);
    int scaled_y = convert_coord(active_chess_pos.y, N_COLS);

    int collision_state = NO_COLLISION;

    int found_other = 0;
    int used_aps = 0;

    while ((scaled_x >= 0 && scaled_x < N_ROWS) && (scaled_y >= 0 && scaled_y < N_COLS)) {
      if (found_other == 1) {
        break;
      }

      if (used_aps > active_piece_aps) {
        break;
      }

      int converted_x = convert_coord(scaled_x, N_ROWS);
      int converted_y = convert_coord(scaled_y, N_COLS);
      Vector3 scaled_pos = calculate_position(converted_x, converted_y, piece_size);

      scaled_x = scaled_x + offset.x;
      scaled_y = scaled_y + offset.y;

      used_aps++;

      if ((collision_state = should_skip_cell(
                         convert_coord(converted_x, N_ROWS),
                         convert_coord(converted_y, N_COLS),
                         player_sign,
                         active_player,
                         cells))) {

        // Check if it's the origin piece first

        if (((scaled_x - offset.x) == origin_x && (scaled_y - offset.y) == origin_y)) {
         continue;
        }
        // Make sure to check if it's our own piece but only after checking if it's the origin
        else if (collision_state == OWN_PIECE) {
          break;
        }
        else if (collision_state == OTHER_PIECE) {
          found_other = 1;
        }
        else {
          break;
        }
      }

      // have to select the *previous* one
      move_chess_pos.x = convert_coord(scaled_x - offset.x, N_ROWS);
      move_chess_pos.y = convert_coord(scaled_y - offset.y, N_COLS);

      if (move_to_count == active_cell_to_move_to) {
        DrawCube(scaled_pos, 5, 0.1f, 5, BLUE);
        active_players.select_to_move_to_chess_positions[active_player] = move_chess_pos;
      }
      else {
        DrawCube(scaled_pos, 5, 0.1f, 5, GREEN);
      }

      move_to_count++;

    }
  }

  return move_to_count;
}

static int
clamp(int d, int min, int max) {
  const int t = d < min ? min : d;
  return t > max ? max : t;
}


static int
calculate_row_move_forward(int active_cell_to_move_to, int player_sign, int move_count, int n_rows) {
  if (move_count <= 0) {
    return 0;
  }
  return clamp(active_cell_to_move_to + (player_sign * n_rows) % move_count, 0, move_count - 1);
}

static int
calculate_row_move_backward(int active_cell_to_move_to, int player_sign, int move_count, int n_rows) {
  if (move_count <= 0) {
    return 0;
  }
  return clamp(active_cell_to_move_to - (player_sign * n_rows) % move_count, 0, move_count - 1);
}

static int
find_next_piece(int active_piece_to_move,
                int active_player,
                int direction,
                struct Cells cells,
                struct ChessPieces pieces) {
  // Cycles through all your active pieces
  // TODO: use a quadtree to do this as well for the mouse
  assert(active_piece_to_move < 16);

  if (direction == 1) {
    for (int i = active_piece_to_move; i < N_PIECES; i++) {
      if (pieces.is_dead[i+1] != 1 && (i+1 < N_PIECES)) {
        return clamp(i+1, 0, N_PIECES-1);
      }
    }
  }
  else if (direction == -1) {
    for (int i = active_piece_to_move; i > 0; i--) {
      if (pieces.is_dead[i-1] != 1 && (i-1 >= 0)) {
        return clamp(i-1, 0, N_PIECES-1);
      }
    }
  }
  return active_piece_to_move; // If we didn't find anything return the original cell, can't return 0 because it could be invalid!
}

int
main(void)
{

    int q_size = next_pow2(next_pow2(N_CELLS*2+1) + 1); // add 1 for the root node
    // Quad-Tree stuff
    Vector2 quad_sizes_buf[q_size];
    Vector3 quad_positions_buf[q_size];
    int quad_piece_indices_buf[q_size];
    int top_left_quad_children[q_size];
    int top_right_quad_children[q_size];
    int bottom_left_quad_children[q_size];
    int bottom_right_quad_children[q_size];

    struct QItem queue[q_size];

    const int screenWidth = 800;
    const int screenHeight = 450;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(screenWidth, screenHeight, "wesgame");
    EnableCursor();

    int display = GetCurrentMonitor();
    //SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));

    rlTPCamera orbitCam;
    rlTPCameraInit(&orbitCam, 45, (Vector3){ 1, 0 ,0 });
    orbitCam.ViewAngles.y = -15 * DEG2RAD;

    load_assets();

    SetTargetFPS(60);

    // Piece type stuff
    struct ChessTypes chess_types = {
      .textures = &piece_textures[0],
      .models = &piece_models[0],
      .scaling_factors = &piece_scaling_factors[0],
      .offset_sizes = &offset_sizes[0],
      .offsets = &offsets[0],
    };

    memset(&white_piece_cell_indices[0], -1, (sizeof white_piece_cell_indices));
    // Gameplay piece stuff
    struct ChessPieces white_pieces = {
      .grid_positions = &white_grid_positions[0],
      .chess_positions = &white_chess_positions[0],
      .is_dead = &white_pieces_dead[0],
      .chess_type = &white_starting_pieces[0],
      .colors = &white_colors[0], // later on, a player could have differently colored pieces
      .action_points_per_turn = &white_starting_aps[0],
      .piece_cell_indices = &white_piece_cell_indices[0]
    };

    memset(&black_piece_cell_indices[0], -1, (sizeof black_piece_cell_indices));
    struct ChessPieces black_pieces = {
      .grid_positions = &black_grid_positions[0],
      .chess_positions = &black_chess_positions[0],
      .is_dead = &black_pieces_dead[0],
      .chess_type = &black_starting_pieces[0],
      .colors = &black_colors[0], // later on, a player could have differently colored pieces
      .action_points_per_turn = &black_starting_aps[0],
      .piece_cell_indices = &black_piece_cell_indices[0]
    };

    // TODO, load these from data, set the size when it loads the players in a level
    // this is kind of like a "pivot" table I think, it helps map from player to their piece sets
    struct ChessPieces pieces[2] = {white_pieces, black_pieces};
    int piece_indices[2] = {0, 1}; // indices mapping to different sets of pieces
    int num_players = (sizeof pieces) / (sizeof (struct ChessPieces));

    // Player type stuff

    int score[2] = {0, 0};
    // TODO allocate these from an arena for variable number of players (and bots / NPCs later)
    PlayerType active_players_buf[2] = {WHITE_PLAYER, BLACK_PLAYER};

    // Tracks the state a player is currently in
    PlayerState player_states_buf[2] = {PIECE_SELECTION, PIECE_SELECTION};
    int select_to_move_pieces_buf[2] = {0, 0};
    int live_piece_counts_buf[2] = {N_PIECES, N_PIECES}; // start out being able to select any piece
    int select_to_move_to_cells_buf[2] = {-1, -1};
    Vector2 select_to_move_to_chess_positions_buf[2] = {{0},{0}};

    struct Players active_players = {
      .score = &score[0],
      .select_to_move_pieces = &select_to_move_pieces_buf[0],
      .select_to_move_to_cells = &select_to_move_to_cells_buf[0],
      .select_to_move_to_chess_positions = &select_to_move_to_chess_positions_buf[0],
      .live_piece_counts = &live_piece_counts_buf[0],
      .player_type = &active_players_buf[0],
      .piece_indices = &piece_indices[0],
      .player_states = &player_states_buf[0]
    };

    // Cell stuff
    memset(&cell_player_states[0], -1, (sizeof cell_player_states));
    struct Cells cells = {
      .occupied_states = &occupied_states[0],
      .cell_player_states = &cell_player_states[0],
      .cell_piece_indices = &cell_piece_indices[0]
    };

    set_pieces(white_pieces, cells, active_players, PIECE_SIZE, TOP_SIDE, WHITE_PLAYER);
    set_pieces(black_pieces, cells, active_players, PIECE_SIZE, BOTTOM_SIDE, BLACK_PLAYER);

    int active_player = BLACK_PLAYER;

    // This is specific to chess moves because they are inverted for either side
    // In some other cell based game, this could be based on a direction variable instead
    // we will want to orient the camera depending on the player as well
    int player_sign = active_player == BLACK_PLAYER ? -1 : 1;

    float time_since_move = 0;

    struct Quads qtree = {
      .quad_sizes = &quad_sizes_buf[0],
      .quad_positions = &quad_positions_buf[0],
      .piece_indices = &quad_piece_indices_buf[0],
      .top_left = &top_left_quad_children[0],
      .top_right = &top_right_quad_children[0],
      .bottom_left = &bottom_left_quad_children[0],
      .bottom_right = &bottom_right_quad_children[0]
    };

    initialize_qtree(qtree, queue, q_size);

    while (!WindowShouldClose()) {
      player_sign = active_player == BLACK_PLAYER ? -1 : 1; // FIXME doesn't work for more than 2 players
      rlTPCameraUpdate(&orbitCam);

      BeginDrawing();

          ClearBackground(RAYWHITE);

          rlTPCameraBeginMode3D(&orbitCam);

              Vector2 mousePos = GetMousePosition();
              float targetWorldZ = 0.0f;
              Vector3 worldPos = rlTPCameraGetScreenToWorld(&orbitCam, mousePos, targetWorldZ);

              //print_vec3(worldPos);

              struct ChessPieces active_pieces = pieces[active_players.piece_indices[active_player]];

              // Get the IDs of the cell to move and the possible cell to move to
              int active_player_state = active_players.player_states[active_player];
              int active_piece_to_move = active_players.select_to_move_pieces[active_player];
              int active_cell_to_move_to = active_players.select_to_move_to_cells[active_player];
              Vector2 active_chess_pos = active_pieces.chess_positions[active_piece_to_move];

              // Get the position of the currently selected cell and highlight it red
              if (active_pieces.is_dead[active_piece_to_move] == 0) {
                Vector3 highlight_pos = active_pieces.grid_positions[active_piece_to_move];
                highlight_pos.y = 0; // Setting the height of it
                DrawCube(highlight_pos, 5, 0.1f, 5, RED);
              }
              else {
                // TODO
                // We have selected a dead piece, reset to the first live piece
              }

              // These are set by the controls to say which cell to move to
              // the names refer to moving in the x or y direction basically
              int move_count = active_players.live_piece_counts[active_player];

              int col_move_to_forward = calculate_row_move_forward(active_cell_to_move_to, player_sign, move_count, 1);
              int col_move_to_back = calculate_row_move_backward(active_cell_to_move_to, player_sign, move_count, 1);

              int next_piece_to_move_forward = find_next_piece(active_piece_to_move, active_player, 1, cells, active_pieces);
              int next_piece_to_move_backward = find_next_piece(active_piece_to_move, active_player, -1, cells, active_pieces);

              int move_to_count = 0;
              move_to_count = handle_moving_piece(PIECE_SIZE,
                                                  active_cell_to_move_to,
                                                  active_piece_to_move,
                                                  active_player,
                                                  player_sign,
                                                  active_pieces,
                                                  active_players,
                                                  active_chess_pos,
                                                  chess_types,
                                                  cells);

              // Handle cell movement for different states here
              switch (active_player_state) {
                case PIECE_MOVE:

                  // Needed to know how to iterate through possible moves
                  active_players.live_piece_counts[active_player] = move_to_count;

                  if (left_x_left_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
                    active_players.select_to_move_to_cells[active_player] = col_move_to_back;
                    time_since_move = 0.0f;
                  }

                  if (left_x_right_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_to_cells[active_player] = col_move_to_forward;
                    time_since_move = 0.0f;
                  }

                  break;
                case PIECE_SELECTION:

                  active_players.select_to_move_to_cells[active_player] = 0;
                  active_players.live_piece_counts[active_player] = N_PIECES;

                  if (left_x_left_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_pieces[active_player] = next_piece_to_move_backward;
                    time_since_move = 0.0f;
                  }

                  if (left_x_right_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_pieces[active_player] = next_piece_to_move_forward;
                    time_since_move = 0.0f;
                  }

                  break;
              }

              if (switch_players_control() && time_since_move >= 0.2f) {
                printf("Switching players\n");
                active_player = (active_player + 1) % NUM_PLAYERS;
                time_since_move = 0.0f;
                continue;
              }

              // Handle switching modes here
              if (trigger_control() && time_since_move >= 0.2f) {
                if (active_player_state == PIECE_MOVE) {
                  active_player_state = active_players.player_states[active_player] = PIECE_SELECTION;
                }
                else if (move_count > 0) {
                  active_player_state = active_players.player_states[active_player] = PIECE_MOVE;
                }
                time_since_move = 0.0f;
              }

              // Handle moving a piece to a new cell here
              if (select_control() && time_since_move >= 0.2f) {
                if (active_player_state == PIECE_MOVE && move_count > 0) {
                  Vector2 chessPosMoveTo = active_players.select_to_move_to_chess_positions[active_player];
                  Vector2 chessPosMoveFrom = active_pieces.chess_positions[active_piece_to_move];

                  int x_to = convert_coord(chessPosMoveTo.x, N_ROWS);
                  int y_to = convert_coord(chessPosMoveTo.y, N_COLS);

                  int x_from = convert_coord(chessPosMoveFrom.x, N_ROWS);
                  int y_from = convert_coord(chessPosMoveFrom.y, N_COLS);

                  if (cells.occupied_states[y_to + (x_to * N_COLS)] == 1) {
                    int kill_cell_piece_index = cells.cell_piece_indices[y_to + (x_to * N_COLS)];
                    int kill_cell_player_id = cells.cell_player_states[y_to + (x_to * N_COLS)];
                    // Now get the player associated and set that piece to be dead
                    pieces[active_players.piece_indices[kill_cell_player_id]].is_dead[kill_cell_piece_index] = 1;
                    active_players.live_piece_counts[kill_cell_player_id]--; // reduce number of live pieces for enemy
                  }

                  // Moving around all the state tracking stuff
                  // This tracks whether a cell is occupied or not
                  cells.occupied_states[y_to + (x_to * N_COLS)] = 1;
                  cells.occupied_states[y_from + (x_from * N_COLS)] = 0;

                  // This tracks which piece is currently occupying a cell
                  cells.cell_piece_indices[y_to + (x_to * N_COLS)] = cells.cell_piece_indices[y_from + (x_from * N_COLS)];
                  cells.cell_piece_indices[y_from + (x_from * N_COLS)] = 0;

                  // This tracks which player is currently occupying a cell
                  cells.cell_player_states[y_to + (x_to * N_COLS)] = active_player;
                  cells.cell_player_states[y_from + (x_from * N_COLS)] = -1;

                  // Set the x,y coordinates first of the piece we want to move
                  active_pieces.chess_positions[active_piece_to_move].x = chessPosMoveTo.x;
                  active_pieces.chess_positions[active_piece_to_move].y = chessPosMoveTo.y;

                  // Then update with the calculated grid position
                  active_pieces.grid_positions[active_piece_to_move] = calculate_position(chessPosMoveTo.x, chessPosMoveTo.y, PIECE_SIZE);

                  // and reset the mode back to piece selection
                  active_player_state = active_players.player_states[active_player] = PIECE_SELECTION;

                }
                time_since_move = 0.0f;
              }

              time_since_move += GetFrameTime();

              for (int player_index = 0; player_index < num_players; player_index++) {
                struct ChessPieces player_pieces = pieces[player_index];
                for (int i = 0; i < N_PIECES; i++) {
                  int is_dead = player_pieces.is_dead[i];

                  if (is_dead) {
                    continue;
                  }

                  Vector3 grid_pos = player_pieces.grid_positions[i];
                  Color piece_color = player_pieces.colors[i];

                  int piece_type = player_pieces.chess_type[i];
                  Model model = chess_types.models[piece_type];
                  float scaling_factor = chess_types.scaling_factors[piece_type];

                  DrawModel(model, grid_pos, scaling_factor, piece_color);
                }
              }

              DrawGrid(MAX(N_ROWS, N_COLS), 5.0f);
          rlTPCameraEndMode3D();

          DrawRectangle( 10, 6, 50, 50, Fade(SKYBLUE, 0.5f));
          DrawRectangleLines( 10, 6, 50, 50, BLUE);

          DrawText("Chess!", 20, 20, 5, BLACK);

      EndDrawing();
    }

    CloseWindow();

    return 0;
}
