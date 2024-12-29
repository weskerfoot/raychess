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

static ChessPiece whiteStartingPieces[N_PIECES] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,           // First row
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK // Second row
};

static ChessPiece blackStartingPieces[N_PIECES] = {
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK, // Second row
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN           // First row
};

static Color blackColors[N_PIECES] = {
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

static Color whiteColors[N_PIECES] = {
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

static int whiteStartingAPs[N_PIECES] = {
    1, 1, 1, 1, 1, 1, 1, 1,     // First row
    N_COLS, 1, N_COLS, N_COLS, 1, N_COLS, 1, N_COLS, // Second row
};

static int blackStartingAPs[N_PIECES] = {
    N_COLS, 1, N_COLS, N_COLS, 1, N_COLS, 1, N_COLS, // Second row
    1, 1, 1, 1, 1, 1, 1, 1     // First row
};

static int white_piece_cell_indices[N_PIECES];
static int black_piece_cell_indices[N_PIECES];

// allocate buffers for game data
static Texture2D pieceTextures[6];
static Model pieceModels[6];
static float pieceScalingFactors[6] = {20.0f, 20.0f, 20.0f, 20.0f, 20.0f, 20.0f};

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

static Vector3 whiteGridPositions[N_PIECES];
static Vector3 blackGridPositions[N_PIECES];
static Vector2 whiteChessPositions[N_PIECES];
static Vector2 blackChessPositions[N_PIECES];
static uint8_t whitePiecesDead[N_PIECES];
static uint8_t blackPiecesDead[N_PIECES];

// Stores a mapping of piece positions to their occupied state
// y + (x * N_COLS) gives you the position in the array
// maybe if the grids get really large it could just do collision detection though?
// that would require computing hit boxes each time you move though which would be slower I think
// not using a multi-dimensional array, might be resizable later
// no, just use multiple cells per entity/object
static uint8_t occupied_states[N_CELLS];
static int cell_player_states[N_CELLS];
static int cell_piece_indices[N_CELLS];

static void
load_assets() {
    pieceModels[PAWN] = LoadModel("resources/models/chess_pieces_models/pawn.glb");
    pieceModels[KNIGHT] = LoadModel("resources/models/chess_pieces_models/knight.glb");
    pieceModels[BISHOP] = LoadModel("resources/models/chess_pieces_models/bishop.glb");
    pieceModels[ROOK] = LoadModel("resources/models/chess_pieces_models/rook.glb");
    pieceModels[QUEEN] = LoadModel("resources/models/chess_pieces_models/queen.glb");
    pieceModels[KING] = LoadModel("resources/models/chess_pieces_models/king.glb");
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

static Vector3
calculate_move(int col, int row, int size) {
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

  for (float i = -3; i <= 4; i++) {
    for (float j = -3; j <= 4; j++) {
      Vector3 position = calculate_move(i, j, size);
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
  assert (cell_id < 65);
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
    printf("position = %d\n", position);
    print_board_state(&cells.occupied_states[0]);
    print_cell_player_states(&cells.cell_player_states[0]);
    printf("x = %d, y = %d\n", x, y);
    return OWN_PIECE;
  }

  if (cells.cell_player_states[position] != active_player && cells.occupied_states[position] != 0) {
    return OTHER_PIECE;
  }

  return NO_COLLISION;
}

static int
handle_moving_piece(struct ChessPieces active_pieces,
                    int piece_size,
                    int active_cell_to_move_to,
                    int active_piece_to_move,
                    int active_player,
                    int player_sign,
                    struct Players active_players,
                    Vector2 active_chess_pos,
                    struct ChessTypes chess_types,
                    struct Cells cells) {

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
      Vector3 scaled_pos = calculate_move(converted_x, converted_y, piece_size);

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
find_next_piece(int active_piece_to_move, int active_player, int direction, struct Cells cells, struct ChessPieces pieces) {
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
    float piece_size = 5.0f;

    // Piece type stuff
    struct ChessTypes chess_types = {
      .textures = &pieceTextures[0],
      .models = &pieceModels[0],
      .scaling_factors = &pieceScalingFactors[0],
      .offset_sizes = &offset_sizes[0],
      .offsets = &offsets[0],
    };

    memset(&white_piece_cell_indices[0], -1, (sizeof white_piece_cell_indices));
    // Gameplay piece stuff
    struct ChessPieces white_pieces = {
      .grid_positions = &whiteGridPositions[0],
      .chess_positions = &whiteChessPositions[0],
      .is_dead = &whitePiecesDead[0],
      .chess_type = &whiteStartingPieces[0],
      .colors = &whiteColors[0], // later on, a player could have differently colored pieces
      .action_points_per_turn = &whiteStartingAPs[0],
      .piece_cell_indices = &white_piece_cell_indices[0]
    };

    memset(&black_piece_cell_indices[0], -1, (sizeof black_piece_cell_indices));
    struct ChessPieces black_pieces = {
      .grid_positions = &blackGridPositions[0],
      .chess_positions = &blackChessPositions[0],
      .is_dead = &blackPiecesDead[0],
      .chess_type = &blackStartingPieces[0],
      .colors = &blackColors[0], // later on, a player could have differently colored pieces
      .action_points_per_turn = &blackStartingAPs[0],
      .piece_cell_indices = &black_piece_cell_indices[0]
    };

    // TODO, load these from data, set the size when it loads the players in a level
    struct ChessPieces pieces[2] = {white_pieces, black_pieces};
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
      .pieces = &pieces[0],
      .player_states = &player_states_buf[0]
    };

    // Cell stuff
    memset(&cell_player_states[0], -1, (sizeof cell_player_states));
    struct Cells cells = {
      .occupied_states = &occupied_states[0],
      .cell_player_states = &cell_player_states[0],
      .cell_piece_indices = &cell_piece_indices[0]
    };

    set_pieces(white_pieces, cells, active_players, piece_size, TOP_SIDE, WHITE_PLAYER);
    set_pieces(black_pieces, cells, active_players, piece_size, BOTTOM_SIDE, BLACK_PLAYER);

    print_board_state(&cells.occupied_states[0]);
    print_cell_player_states(&cells.cell_player_states[0]);

    int active_player = BLACK_PLAYER;

    // This is specific to chess moves because they are inverted for either side
    // In some other cell based game, this could be based on a direction variable instead
    // we will want to orient the camera depending on the player as well
    int player_sign = active_player == BLACK_PLAYER ? -1 : 1;

    float time_since_move = 0;

    while (!WindowShouldClose()) {
      player_sign = active_player == BLACK_PLAYER ? -1 : 1; // FIXME doesn't work for more than 2 players
      rlTPCameraUpdate(&orbitCam);

      BeginDrawing();

          ClearBackground(RAYWHITE);

          rlTPCameraBeginMode3D(&orbitCam);

              struct ChessPieces active_pieces = active_players.pieces[active_player];

              // Get the IDs of the cell to move and the possible cell to move to
              int active_player_state = active_players.player_states[active_player];
              int active_piece_to_move = active_players.select_to_move_pieces[active_player];
              int active_cell_to_move_to = active_players.select_to_move_to_cells[active_player];
              Vector2 active_chess_pos = active_pieces.chess_positions[active_piece_to_move];

              // Get the position of the currently selected cell and highlight it red
              Vector3 highlight_pos = active_pieces.grid_positions[active_piece_to_move];
              highlight_pos.y = 0; // Setting the height of it
              DrawCube(highlight_pos, 5, 0.1f, 5, RED);

              // These are set by the controls to say which cell to move to
              // the names refer to moving in the x or y direction basically
              int move_count = active_players.live_piece_counts[active_player];

              // FIXME, this should modulo arithmetic over the table of active pieces for the player and get the next live one
              int col_move_to_forward = calculate_row_move_forward(active_cell_to_move_to, player_sign, move_count, 1);
              int col_move_to_back = calculate_row_move_backward(active_cell_to_move_to, player_sign, move_count, 1);

              int next_piece_to_move_forward = find_next_piece(active_piece_to_move, active_player, 1, cells, active_pieces);
              int next_piece_to_move_backward = find_next_piece(active_piece_to_move, active_player, -1, cells, active_pieces);

              // FIXME reduce number of parameters
              int move_to_count = 0;
              move_to_count = handle_moving_piece(active_pieces,
                                                  piece_size,
                                                  active_cell_to_move_to,
                                                  active_piece_to_move,
                                                  active_player,
                                                  player_sign,
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
                  active_players.live_piece_counts[active_player] = N_PIECES; // FIXME this needs to be the number of live pieces I think?

                  if (left_x_left_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
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
                  int y_to = convert_coord(chessPosMoveTo.y, N_ROWS);

                  int x_from = convert_coord(chessPosMoveFrom.x, N_ROWS);
                  int y_from = convert_coord(chessPosMoveFrom.y, N_ROWS);

                  if (cells.occupied_states[y_to + (x_to * N_COLS)] == 1) {
                    // FIXME, when I kill a piece, I need to reset the state on the player I kill too so they won't be selecting a dead piece
                    // get the piece associated with the cell currently
                    int kill_cell_piece_index = cells.cell_piece_indices[y_to + (x_to * N_COLS)];
                    int kill_cell_player_id = cells.cell_player_states[y_to + (x_to * N_COLS)];
                    printf("killing a piece from player %d at index %d\n", kill_cell_player_id, kill_cell_piece_index);
                    // Now get the player associated and set that piece to be dead
                    active_players.pieces[kill_cell_player_id].is_dead[kill_cell_piece_index] = 1;
                    active_players.live_piece_counts[kill_cell_player_id]--; // reduce number of live pieces for enemy
                  }

                  cells.occupied_states[y_to + (x_to * N_COLS)] = 1;
                  cells.occupied_states[y_from + (x_from * N_COLS)] = 0;

                  cells.cell_piece_indices[y_to + (x_to * N_COLS)] = cells.cell_piece_indices[y_from + (x_from * N_COLS)];
                  cells.cell_piece_indices[y_from + (x_from * N_COLS)] = 0;

                  cells.cell_player_states[y_to + (x_to * N_COLS)] = active_player;
                  cells.cell_player_states[y_from + (x_from * N_COLS)] = -1;

                  print_board_state(&cells.occupied_states[0]);
                  print_cell_player_states(&cells.cell_player_states[0]);

                  // Set the x,y coordinates first of the piece we want to move
                  active_pieces.chess_positions[active_piece_to_move].x = chessPosMoveTo.x;
                  active_pieces.chess_positions[active_piece_to_move].y = chessPosMoveTo.y;

                  // Then update with the calculated grid position
                  active_pieces.grid_positions[active_piece_to_move] = calculate_move(chessPosMoveTo.x, chessPosMoveTo.y, piece_size);

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

              DrawGrid(N_ROWS, 5.0f);
          rlTPCameraEndMode3D();

          DrawRectangle( 10, 6, 50, 50, Fade(SKYBLUE, 0.5f));
          DrawRectangleLines( 10, 6, 50, 50, BLUE);

          DrawText("Chess!", 20, 20, 5, BLACK);

      EndDrawing();
    }

    CloseWindow();

    return 0;
}
