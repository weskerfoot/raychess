#include "stdint.h"
#include "raylib.h"
#include "math.h"
#include "stdio.h"
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

static ChessPieceMovement movement_types[6] = {
  NORMAL_MOVEMENT,
  NORMAL_MOVEMENT,
  UNBOUNDED,
  UNBOUNDED,
  UNBOUNDED,
  NORMAL_MOVEMENT
};

static Vector3 whiteGridPositions[N_PIECES];
static Vector3 blackGridPositions[N_PIECES];
static Vector2 whiteChessPositions[N_PIECES];
static Vector2 blackChessPositions[N_PIECES];
static uint8_t white_piecesDead[N_PIECES];
static uint8_t black_piecesDead[N_PIECES];

// Stores a mapping of piece positions to their occupied state
// y + (x * N_COLS) gives you the position in the array
// maybe if the grids get really large it could just do collision detection though?
// that would require computing hit boxes each time you move though which would be slower I think
// not using a multi-dimensional array, might be resizable later
// no, just use multiple cells per entity/object
static uint8_t occupied_states[N_CELLS];
static int cell_player_states[N_CELLS];

static void
loadAssets() {
    pieceModels[PAWN] = LoadModel("resources/models/chess_pieces_models/pawn.glb");
    pieceModels[KNIGHT] = LoadModel("resources/models/chess_pieces_models/knight.glb");
    pieceModels[BISHOP] = LoadModel("resources/models/chess_pieces_models/bishop.glb");
    pieceModels[ROOK] = LoadModel("resources/models/chess_pieces_models/rook.glb");
    pieceModels[QUEEN] = LoadModel("resources/models/chess_pieces_models/queen.glb");
    pieceModels[KING] = LoadModel("resources/models/chess_pieces_models/king.glb");
    return;
}

static int
convertCoord(int input, int n) {
  // n = number of cells in a row or column
  // Translate coordinates
  assert (n > 0);
  int half = n / 2;
  return half - input;
}

static void
printVec2(Vector2 vec) {
  printf("x = %f, y = %f\n", vec.x, vec.y);
}

static void
printVec3(Vector3 vec) {
  printf("x = %f, y = %f, z = %f\n", vec.x, vec.y, vec.z);
}

static void
printBoardState(uint8_t *board) {
  for (int i = 0; i < N_CELLS; i++) {
    printf("%d", board[i]);
  }
  printf("\n");
}

static Vector3
calculateMove(int col, int row, int size) {
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
setPieces(struct ChessPieces pieces, struct Cells cells, int size, unsigned int side) {
  int piece = 0;
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
      Vector3 position = calculateMove(i, j, size);
      // there are always 16 pieces per player
      if (piece >= start && piece < end) {
        pieces.grid_positions[piece % N_PIECES] = position;
        pieces.chess_positions[piece % N_PIECES] = (Vector2){i, j};
        pieces.is_dead[piece % N_PIECES] = 0;
        cells.occupied_states[piece] = 1;
      }
      piece++;
    }
  }
  return pieces;
}

static int
shouldSkipCell(int x,
               int y,
               int player_sign,
               struct Cells cells) {
  // Filter out moves off the end of the board
  if (x < 0 || y < 0 || x >= N_ROWS || y >= N_COLS) {
    return 1;
  }

  // Check if the position is occupied already, TODO only check for your own colour
  if (cells.occupied_states[y + (x * N_COLS)] == 1) {
    return 1;
  }

  return 0;
}

static int
handleMovementsUnbounded(struct ChessPieces active_pieces,
                         int piece_size,
                         int active_cell_to_move_to,
                         int active_cell_to_move,
                         int active_player,
                         int player_sign,
                         struct Players active_players,
                         Vector2 active_chess_pos,
                         struct ChessTypes chess_types,
                         struct Cells cells) {
  // FIXME get this in here or pass it in
  //ChessPieceMovement active_piece_movement_type = chess_types.movement_types[active_piece_type];
  int move_to_count = 0;

  int active_piece_type = active_pieces.chess_type[active_cell_to_move];

  Vector2 *offsets = chess_types.offsets[active_piece_type];
  int offsetNum = chess_types.offset_sizes[active_piece_type];

  // FIXME check if it should be scaled here
  for (int offsetIndex = 0; offsetIndex < offsetNum; offsetIndex++) {
    Vector2 offset = offsets[offsetIndex];

    Vector2 move_chess_pos;

    int current_x = convertCoord(active_chess_pos.x, N_ROWS);
    int current_y = convertCoord(active_chess_pos.y, N_COLS);

    int new_x = current_x + (offset.x * player_sign);
    int new_y = current_y + (offset.y * player_sign);

    if ((offset.x * N_ROWS) != 0.0f || (offset.y * N_COLS) != 0.0f) {

      float scaled_x = current_x;
      float scaled_y = current_y;

      while ((scaled_x >= 0 && scaled_x < N_ROWS) &&
             (scaled_y >= 0 && scaled_y < N_COLS)) {
          int converted_x = convertCoord(scaled_x, N_ROWS);
          int converted_y = convertCoord(scaled_y, N_COLS);
          Vector3 scaled_pos = calculateMove(converted_x, converted_y, piece_size);

          scaled_x = scaled_x + offset.x;
          scaled_y = scaled_y + offset.y;

          //int move_to_player_type = active_pieces.chess_type[active_cell_to_move];

          if (shouldSkipCell(convertCoord(converted_x, N_ROWS),
                             convertCoord(converted_y, N_COLS),
                             player_sign,
                             cells)) {

            // need to move the position regardless
            move_chess_pos.x = convertCoord(scaled_x, N_ROWS);
            move_chess_pos.y = convertCoord(scaled_y, N_COLS);

            // Check if it's the origin piece first
            if (!((scaled_x - offset.x) == current_x && (scaled_y - offset.y) == current_y)) {
              break;
            }
            continue;
          }

          if (move_to_count == active_cell_to_move_to) {
            DrawCube(scaled_pos, 5, 0.1f, 5, BLUE);
            active_players.select_to_move_to_chess_positions[active_player] = move_chess_pos;
          }
          else {
            DrawCube(scaled_pos, 5, 0.1f, 5, GREEN);
          }

          move_to_count++;
          move_chess_pos.x = convertCoord(scaled_x, N_ROWS);
          move_chess_pos.y = convertCoord(scaled_y, N_COLS);
      }
    }

    // Note x and y will never both be 0

    if (shouldSkipCell(new_x, new_y, player_sign, cells)) {
      continue;
    }

    move_chess_pos.x = convertCoord(new_x, N_ROWS);
    move_chess_pos.y = convertCoord(new_y, N_COLS);

    Vector3 move_position = calculateMove(move_chess_pos.x, move_chess_pos.y, piece_size);

    if (move_to_count == active_cell_to_move_to) {
      DrawCube(move_position, 5, 0.1f, 5, BLUE);
      active_players.select_to_move_to_chess_positions[active_player] = move_chess_pos;
    }
    else {
      DrawCube(move_position, 5, 0.1f, 5, GREEN);
    }

    move_to_count++;
  }

  assert (move_to_count >= 0);
  return move_to_count;
}

static int
handleMovements(struct ChessPieces active_pieces,
                int piece_size,
                int active_cell_to_move_to,
                int active_cell_to_move,
                int active_player,
                int player_sign,
                struct Players active_players,
                Vector2 active_chess_pos,
                struct ChessTypes chess_types,
                struct Cells cells) {
  int move_to_count = 0;

  int active_piece_type = active_pieces.chess_type[active_cell_to_move];

  Vector2 *offsets = chess_types.offsets[active_piece_type];
  int offsetNum = chess_types.offset_sizes[active_piece_type];

  // This loop handles highlighting the possible cells the currently selected piece could move to
  // It also sets select_to_move_to_chess_positions for the current player
  // which is later used to actually move to that position if they decide to
  for (int offsetIndex = 0; offsetIndex < offsetNum; offsetIndex++) {
    Vector2 offset = offsets[offsetIndex];
    Vector2 move_chess_pos;

    int new_x = convertCoord(active_chess_pos.x, N_ROWS) + (offset.x * player_sign);
    int new_y = convertCoord(active_chess_pos.y, N_COLS) + (offset.y * player_sign);

    if (shouldSkipCell(new_x, new_y, player_sign, cells)) {
      continue;
    }

    move_chess_pos.x = convertCoord(new_x, N_ROWS);
    move_chess_pos.y = convertCoord(new_y, N_COLS);

    Vector3 move_position = calculateMove(move_chess_pos.x, move_chess_pos.y, piece_size);

    if (move_to_count == active_cell_to_move_to) {
      DrawCube(move_position, 5, 0.1f, 5, BLUE);
      // Sets the chess position of the cell this player is possibly moving to
      active_players.select_to_move_to_chess_positions[active_player] = move_chess_pos;
    }
    else {
      DrawCube(move_position, 5, 0.1f, 5, GREEN);
    }

    move_to_count++;
  }
  return move_to_count;
}

static int
clamp(int d, int min, int max) {
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

int
main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera free");

    rlTPCamera orbitCam;
    rlTPCameraInit(&orbitCam, 45, (Vector3){ 1, 0 ,0 });
    orbitCam.ViewAngles.y = -15 * DEG2RAD;

    loadAssets();

    DisableCursor();

    SetTargetFPS(60);
    float piece_size = 5.0f;

    // Piece type stuff
    struct ChessTypes chess_types = {
      .textures = &pieceTextures[0],
      .models = &pieceModels[0],
      .scaling_factors = &pieceScalingFactors[0],
      .offset_sizes = &offset_sizes[0],
      .offsets = &offsets[0],
      .movement_types = &movement_types[0]
    };

    // Gameplay piece stuff
    struct ChessPieces white_pieces = {
      .grid_positions = &whiteGridPositions[0],
      .chess_positions = &whiteChessPositions[0],
      .is_dead = &white_piecesDead[0],
      .chess_type = &whiteStartingPieces[0],
      .colors = &whiteColors[0] // later on, a player could have differently colored pieces
    };

    struct ChessPieces black_pieces = {
      .grid_positions = &blackGridPositions[0],
      .chess_positions = &blackChessPositions[0],
      .is_dead = &black_piecesDead[0],
      .chess_type = &blackStartingPieces[0],
      .colors = &blackColors[0] // later on, a player could have differently colored pieces
    };

    // TODO, load these from data, set the size when it loads the players in a level
    struct ChessPieces pieces[2] = {white_pieces, black_pieces};
    int num_players = (sizeof pieces) / (sizeof (struct ChessPieces));

    // Player type stuff

    int score[2] = {0, 0};
    // TODO allocate these from an arena for variable number of players (and bots / NPCs later)
    PlayerType active_players_buf[2] = {WHITE_PLAYER, BLACK_PLAYER};
    PlayerState player_states_buf[2] = {PIECE_SELECTION, PIECE_SELECTION};
    int select_to_move_cells_buf[2] = {0, 0};
    int possible_move_counts_buf[2] = {N_PIECES, N_PIECES}; // start out being able to select any piece
    int select_to_move_to_cells_buf[2] = {0, 0};
    Vector2 select_to_move_to_chess_positions_buf[2] = {{0},{0}};

    struct Players active_players = {
      .score = &score[0],
      .select_to_move_cells = &select_to_move_cells_buf[0],
      .select_to_move_to_cells = &select_to_move_to_cells_buf[0],
      .select_to_move_to_chess_positions = &select_to_move_to_chess_positions_buf[0],
      .possible_move_counts = &possible_move_counts_buf[0],
      .player_type = &active_players_buf[0],
      .pieces = &pieces[0],
      .player_states = &player_states_buf[0]
    };

    // Cell stuff
    struct Cells cells = {
      .occupied_states = &occupied_states[0],
      .cell_player_states = &cell_player_states[0]
    };

    setPieces(white_pieces, cells, piece_size, TOP_SIDE);
    setPieces(black_pieces, cells, piece_size, BOTTOM_SIDE);

    printBoardState(&cells.occupied_states[0]);

    int active_player = BLACK_PLAYER;

    // This is specific to chess moves because they are inverted for either side
    // In some other cell based game, this could be based on a direction variable instead
    // we will want to orient the camera depending on the player as well
    int player_sign = active_player == BLACK_PLAYER ? -1 : 1;

    float time_since_move = 0;

    while (!WindowShouldClose()) {
      player_sign = active_player == BLACK_PLAYER ? -1 : 1;
      rlTPCameraUpdate(&orbitCam);

      BeginDrawing();

          ClearBackground(RAYWHITE);

          rlTPCameraBeginMode3D(&orbitCam);

              struct ChessPieces active_pieces = active_players.pieces[active_player];

              // Get the IDs of the cell to move and the possible cell to move to
              int active_player_state = active_players.player_states[active_player];
              int active_cell_to_move = active_players.select_to_move_cells[active_player];
              int active_cell_to_move_to = active_players.select_to_move_to_cells[active_player];
              Vector2 active_chess_pos = active_pieces.chess_positions[active_cell_to_move];

              // TODO these are accessed again in the handleMovements functions so maybe those should just be one function?
              int active_piece_type = active_pieces.chess_type[active_cell_to_move];
              ChessPieceMovement active_piece_movement_type = chess_types.movement_types[active_piece_type];

              // Get the position of the currently selected cell and highlight it red
              Vector3 highlight_pos = active_pieces.grid_positions[active_cell_to_move];
              highlight_pos.y = 0; // Setting the height of it
              DrawCube(highlight_pos, 5, 0.1f, 5, RED);

              // These are set by the controls to say which cell to move to
              int move_count = active_players.possible_move_counts[active_player];
              assert (move_count > 0);
              int row_move_to_back = clamp(active_cell_to_move_to - (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int row_move_to_forward = clamp(active_cell_to_move_to + (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int col_move_to_forward = clamp(active_cell_to_move_to + (player_sign * 1) % move_count, 0, move_count - 1);
              int col_move_to_back = clamp(active_cell_to_move_to - (player_sign * 1) % move_count, 0, move_count - 1);

              // These are set by the controls to say which cell to select
              int row_move_back = clamp(active_cell_to_move - (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int row_move_forward = clamp(active_cell_to_move + (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int col_move_forward = clamp(active_cell_to_move + (player_sign * 1) % move_count, 0, move_count - 1);
              int col_move_back = clamp(active_cell_to_move - (player_sign * 1) % move_count, 0, move_count - 1);

              // FIXME reduce number of parameters
              int move_to_count = 0;
              switch (active_piece_movement_type) {
                case NORMAL_MOVEMENT:
                  move_to_count = handleMovements(active_pieces,
                                                  piece_size,
                                                  active_cell_to_move_to,
                                                  active_cell_to_move,
                                                  active_player,
                                                  player_sign,
                                                  active_players,
                                                  active_chess_pos,
                                                  chess_types,
                                                  cells);
                  break;
                case UNBOUNDED:
                  move_to_count = handleMovementsUnbounded(active_pieces,
                                                           piece_size,
                                                           active_cell_to_move_to,
                                                           active_cell_to_move,
                                                           active_player,
                                                           player_sign,
                                                           active_players,
                                                           active_chess_pos,
                                                           chess_types,
                                                           cells);
                  break;

              }

              // Handle cell movement for different states here
              switch (active_player_state) {
                case PIECE_MOVE:

                  // Needed to know how to iterate through possible moves
                  active_players.possible_move_counts[active_player] = move_to_count;

                  if (left_x_left_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
                    active_players.select_to_move_to_cells[active_player] = col_move_to_back;
                    time_since_move = 0.0f;
                  }

                  if (left_x_right_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_to_cells[active_player] = col_move_to_forward;
                    time_since_move = 0.0f;
                  }

                  if (left_y_up_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
                    active_players.select_to_move_to_cells[active_player] = row_move_to_back;
                    time_since_move = 0.0f;
                  }

                  if (left_y_down_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_to_cells[active_player] = row_move_to_forward;
                    time_since_move = 0.0f;
                  }
                  break;
                case PIECE_SELECTION:

                  active_players.select_to_move_to_cells[active_player] = 0;
                  active_players.possible_move_counts[active_player] = N_PIECES;

                  if (left_x_left_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
                    active_players.select_to_move_cells[active_player] = col_move_back;
                    time_since_move = 0.0f;
                  }

                  if (left_x_right_control() && time_since_move >= 0.2f) {
                    active_players.select_to_move_cells[active_player] = col_move_forward;
                    time_since_move = 0.0f;
                  }

                  if (left_y_up_control() && time_since_move >= 0.2f) {
                    // FIXME only select live ones?
                    active_players.select_to_move_cells[active_player] = row_move_back;
                    time_since_move = 0.0f;
                  }

                  if (left_y_down_control() &&
                      time_since_move >= 0.2f) {
                    active_players.select_to_move_cells[active_player] = row_move_forward;
                    time_since_move = 0.0f;
                  }
                  break;
              }

              // Handle switching modes here
              if (trigger_control() && time_since_move >= 0.2f) {
                if (active_player_state == PIECE_MOVE) {
                  active_player_state = active_players.player_states[active_player] = PIECE_SELECTION;
                }
                else {
                  active_player_state = active_players.player_states[active_player] = PIECE_MOVE;
                }
                time_since_move = 0.0f;
              }

              // Handle moving a piece to a new cell here
              if (select_control() && time_since_move >= 0.2f) {
                if (active_player_state == PIECE_MOVE) {
                  Vector2 chessPosMoveTo = active_players.select_to_move_to_chess_positions[active_player];
                  Vector2 chessPosMoveFrom = active_pieces.chess_positions[active_cell_to_move];

                  int x_to = convertCoord(chessPosMoveTo.x, N_ROWS);
                  int y_to = convertCoord(chessPosMoveTo.y, N_ROWS);

                  int x_from = convertCoord(chessPosMoveFrom.x, N_ROWS);
                  int y_from = convertCoord(chessPosMoveFrom.y, N_ROWS);

                  //printf("from: x = %d, y = %d, x*y = %d\n", x_from, y_from, y_from + (x_from * N_COLS));
                  //printf("to: x = %d, y = %d, x*y = %d\n", x_to, y_to, y_to + (x_to * N_COLS));

                  cells.occupied_states[y_to + (x_to * N_COLS)] = 1;
                  cells.occupied_states[y_from + (x_from * N_COLS)] = 0;

                  // Set the x,y coordinates first of the piece we want to move
                  active_pieces.chess_positions[active_cell_to_move].x = chessPosMoveTo.x;
                  active_pieces.chess_positions[active_cell_to_move].y = chessPosMoveTo.y;

                  // Then update with the calculated grid position
                  active_pieces.grid_positions[active_cell_to_move] = calculateMove(chessPosMoveTo.x, chessPosMoveTo.y, piece_size);

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
