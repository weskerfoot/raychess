#include "raylib.h"
#include "math.h"
#include "stdio.h"
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

static ChessPiece whiteStartingPieces[N_PIECES] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,           // First row
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK // Second row
};

static ChessPiece blackStartingPieces[N_PIECES] = {
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK, // Second row
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN           // First row
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

static Vector3 whiteGridPositions[N_PIECES];
static Vector3 blackGridPositions[N_PIECES];
static Vector2 whiteChessPositions[N_PIECES];
static Vector2 blackChessPositions[N_PIECES];
static char whitePiecesDead[N_PIECES];
static char blackPiecesDead[N_PIECES];
static Vector3 grid_positions[N_CELLS];

void
loadAssets() {
    pieceModels[PAWN] = LoadModel("resources/models/chess_pieces_models/pawn.glb");
    pieceModels[KNIGHT] = LoadModel("resources/models/chess_pieces_models/knight.glb");
    pieceModels[BISHOP] = LoadModel("resources/models/chess_pieces_models/bishop.glb");
    pieceModels[ROOK] = LoadModel("resources/models/chess_pieces_models/rook.glb");
    pieceModels[QUEEN] = LoadModel("resources/models/chess_pieces_models/queen.glb");
    pieceModels[KING] = LoadModel("resources/models/chess_pieces_models/king.glb");
    return;
}

int
convertCoord(int input, int n) {
  // n = number of cells in a row or column
  // Translate coordinates
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

void
generatePositions(int pieceSize) {
  int piece = 0;
  for (float i = -3; i <= 4; i++) {
    for (float j = -3; j <= 4; j++) {
      Vector3 position = calculateMove(i, j, pieceSize);
      // there are always 16 pieces per player
      if (piece < N_CELLS) {
        grid_positions[piece] = position;
      }
      piece++;
    }
  }
}

static Vector3
calculateMove(int col, int row, int size) {
  // Given a column and row, and a tile size
  // calculate a board position
  Vector3 position = { (size*row) - (size/2.0),
                       0.0f,
                       (size*col) - (size/2.0)}; // 4 = half the grid from center
  return position;
}

struct ChessPieces
setPieces(struct ChessPieces pieces, int size, unsigned int side) {
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
      if (piece >= start && piece < end) { // there must be a nicer way of checking this
        pieces.cells.grid_positions[piece % N_PIECES] = position; // get rid of this and just do a lookup now?
        pieces.cells.chess_positions[piece % N_PIECES] = (Vector2){i, j};
        pieces.is_dead[piece % N_PIECES] = 0;
      }
      piece++;
    }
  }
  return pieces;
}

int
clamp(int d, int min, int max) {
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

int
main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera free");

    rlTPCamera orbitCam;
    rlTPCameraInit(&orbitCam, 45, (Vector3){ 1, 0 ,0 });
    orbitCam.ViewAngles.y = -15 * DEG2RAD;

    loadAssets();

    DisableCursor();

    SetTargetFPS(60);
    float pieceSize = 5.0f;

    // Piece type stuff
    struct ChessTypes chessTypes = {
      .textures = &pieceTextures[0],
      .models = &pieceModels[0],
      .scaling_factors = &pieceScalingFactors[0],
      .offset_sizes = &offset_sizes[0],
      .offsets = &offsets[0]
    };

    // Cell type stuff
    struct Cells white_cells = {
      .grid_positions = &whiteGridPositions[0],
      .chess_positions = &whiteChessPositions[0],
    };

    struct Cells black_cells = {
      .grid_positions = &blackGridPositions[0],
      .chess_positions = &blackChessPositions[0],
    };

    // Gameplay piece stuff
    struct ChessPieces whitePieces = {
      .cells = white_cells,
      .is_dead = &whitePiecesDead[0],
      .chess_type = &whiteStartingPieces[0]
    };

    struct ChessPieces blackPieces = {
      .cells = black_cells,
      .is_dead = &blackPiecesDead[0],
      .chess_type = &blackStartingPieces[0]
    };

    struct ChessPieces pieces[2] = {whitePieces, blackPieces};

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

    setPieces(whitePieces, pieceSize, TOP_SIDE);
    setPieces(blackPieces, pieceSize, BOTTOM_SIDE);

    int active_player = WHITE_PLAYER;

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

              struct ChessPieces activePieces = active_players.pieces[active_player];

              int activePlayerState = active_players.player_states[active_player];
              int active_cell_to_move = active_players.select_to_move_cells[active_player];
              int active_cell_to_move_to = active_players.select_to_move_to_cells[active_player];
              int activePieceType = activePieces.chess_type[active_cell_to_move];
              Vector2 active_chess_pos = activePieces.cells.chess_positions[active_cell_to_move];

              Vector2 *offsets = chessTypes.offsets[activePieceType];
              int offsetNum = chessTypes.offset_sizes[activePieceType];

              int move_to_count = 0;
              for (int offsetIndex = 0; offsetIndex < offsetNum; offsetIndex++) {
                Vector2 offset = offsets[offsetIndex];
                Vector2 move_chess_pos;
                // This is kind of janky, might decide to store the transformed coordinates and just look them up?
                int new_x = convertCoord(active_chess_pos.x, N_ROWS) + (offset.x * player_sign);
                int new_y = convertCoord(active_chess_pos.y, N_COLS) + (offset.y * player_sign);

                // Filter out moves off the end of the board
                if (new_x < 0 || new_y < 0 || new_x >= N_ROWS || new_y >= N_COLS) {
                  continue;
                }

                move_chess_pos.x = convertCoord(new_x, N_ROWS);
                move_chess_pos.y = convertCoord(new_y, N_COLS);

                Vector3 move_position = calculateMove(move_chess_pos.x, move_chess_pos.y, pieceSize);

                if (move_to_count == active_cell_to_move_to) {
                  DrawCube(move_position, 5, 0.1f, 5, BLUE);
                  active_players.select_to_move_to_chess_positions[active_player] = move_chess_pos;
                }
                else {
                  DrawCube(move_position, 5, 0.1f, 5, GREEN);
                }
                move_to_count++;
              }

              switch (activePlayerState) {
                case PIECE_MOVE:
                  active_players.possible_move_counts[active_player] = move_to_count;
                  break;
                case PIECE_SELECTION:
                  active_players.select_to_move_to_cells[active_player] = 0;
                  active_players.possible_move_counts[active_player] = N_PIECES;
                  break;
              }

              Vector3 highlight_pos = activePieces.cells.grid_positions[active_cell_to_move];
              highlight_pos.y = 0; // Setting the height of it
              DrawCube(highlight_pos, 5, 0.1f, 5, RED);

              // Control handling depends on player state
              int move_count = active_players.possible_move_counts[active_player];
              int row_move_to_back = clamp(active_cell_to_move_to - (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int row_move_to_forward = clamp(active_cell_to_move_to + (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int col_move_to_forward = clamp(active_cell_to_move_to + (player_sign * 1) % move_count, 0, move_count - 1);
              int col_move_to_back = clamp(active_cell_to_move_to - (player_sign * 1) % move_count, 0, move_count - 1);

              int row_move_back = clamp(active_cell_to_move - (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int row_move_forward = clamp(active_cell_to_move + (player_sign * N_ROWS) % move_count, 0, move_count - 1);
              int col_move_forward = clamp(active_cell_to_move + (player_sign * 1) % move_count, 0, move_count - 1);
              int col_move_back = clamp(active_cell_to_move - (player_sign * 1) % move_count, 0, move_count - 1);

              // Handle cell movement for different states here
              switch (activePlayerState) {
                case PIECE_MOVE:
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
              if (IsGamepadButtonDown(NINTENDO_CONTROLLER, GAMEPAD_BUTTON_LEFT_TRIGGER_2) && time_since_move >= 0.2f) {
                if (activePlayerState == PIECE_MOVE) {
                  activePlayerState = active_players.player_states[active_player] = PIECE_SELECTION;
                }
                else {
                  activePlayerState = active_players.player_states[active_player] = PIECE_MOVE;
                }
                time_since_move = 0.0f;
              }

              if (IsGamepadButtonDown(NINTENDO_CONTROLLER, GAMEPAD_BUTTON_RIGHT_FACE_UP) && time_since_move >= 0.2f) {
                if (activePlayerState == PIECE_MOVE) {
                  Vector2 chessPosMoveTo = active_players.select_to_move_to_chess_positions[active_player];
                  activePieces.cells.chess_positions[active_cell_to_move].x = chessPosMoveTo.x;
                  activePieces.cells.chess_positions[active_cell_to_move].y = chessPosMoveTo.y;

                  int cell_to_move_x = activePieces.cells.chess_positions[active_cell_to_move].x;
                  int cell_to_move_y = activePieces.cells.chess_positions[active_cell_to_move].y;
                  activePieces.cells.grid_positions[active_cell_to_move] = calculateMove(cell_to_move_x, cell_to_move_y, pieceSize);

                }
                time_since_move = 0.0f;
              }

              time_since_move += GetFrameTime();

              // FIXME should be the number of *live* pieces
              for (int i = 0; i < N_PIECES; i++) {
                Vector3 gridPos = white_cells.grid_positions[i];
                int pieceType = whitePieces.chess_type[i];
                Model model = chessTypes.models[pieceType];
                float scaling_factor = chessTypes.scaling_factors[pieceType];
                DrawModel(model, gridPos, scaling_factor, WHITE);
              }

              // FIXME should be the number of *live* pieces
              for (int i = 0; i < N_PIECES; i++) {
                Vector3 gridPos = black_cells.grid_positions[i];
                int pieceType = blackPieces.chess_type[i];
                Model model = chessTypes.models[pieceType];
                float scaling_factor = chessTypes.scaling_factors[pieceType];
                DrawModel(model, gridPos, scaling_factor, BLACK);
              }

              DrawGrid(N_ROWS, 5.0f);
          rlTPCameraEndMode3D();

          DrawRectangle( 10, 6, 50, 50, Fade(SKYBLUE, 0.5f));
          DrawRectangleLines( 10, 6, 50, 50, BLUE);

          DrawText("Chess!", 20, 20, 5, BLACK);

      EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
