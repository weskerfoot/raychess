#include "raylib.h"
#include "math.h"
#include "stdio.h"
#include "chess.h"
#include "camera/rlTPCamera.h"

const int NINTENDO_CONTROLLER = 1;

static int whiteStartingPieces[16] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,           // First row
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK // Second row
};

static int blackStartingPieces[16] = {
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK, // Second row
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN           // First row
};

// allocate buffers for game data
static Texture2D pieceTextures[6];
static Model pieceModels[6];
static float pieceScalingFactors[6] = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f};

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

static Vector3 whiteGridPositions[16];
static Vector3 blackGridPositions[16];
static Vector2 whiteChessPositions[16];
static Vector2 blackChessPositions[16];
static char whitePiecesDead[16];
static char blackPiecesDead[16];

static Vector3 grid_positions[64];

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
      if (piece < 64) {
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
    end = 16;
  }

  else {
    start = 48;
    end = 64;
  }

  for (float i = -3; i <= 4; i++) {
    for (float j = -3; j <= 4; j++) {
      Vector3 position = calculateMove(i, j, size);
      // there are always 16 pieces per player
      if (piece >= start && piece < end) { // there must be a nicer way of checking this
        pieces.cells.grid_positions[piece % 16] = position; // get rid of this and just do a lookup now?
        pieces.cells.chess_positions[piece % 16] = (Vector2){i, j};
        pieces.is_dead[piece % 16] = 0;
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

void
calculateMovable(struct Players players,
                 int player,
                 struct ChessPieces playerPieces,
                 int activePiece) {
  // Takes in players, a handle to a player, and that player's pieces, and an active piece
  // sets the movable cells on the player to a subset of the grid
  // then later code can simply iterate over the movable cells and highlight them, or set the position to move to
  // based on an index into that array
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
    int active_players[2] = {WHITE_PLAYER, BLACK_PLAYER};

    struct Players players = {
      .score = &score[0],
      .player_type = &active_players[0],
      .pieces = &pieces[0]
    };

    setPieces(whitePieces, pieceSize, TOP_SIDE);
    setPieces(blackPieces, pieceSize, BOTTOM_SIDE);

    int active_chess_piece = 0;
    int active_player = WHITE_PLAYER;

    // Need to have inverted movements for one side vs the other
    //blackPieces.chess_positions[0].x = convertCoord(2, 8);
    //blackPieces.chess_positions[0].y = convertCoord(0, 8);
    //blackPieces.grid_positions[0] = calculateMove(blackPieces.chess_positions[0].x, blackPieces.chess_positions[0].y, pieceSize);

    float time_since_move = 0;

    while (!WindowShouldClose()) {
        rlTPCameraUpdate(&orbitCam);

        BeginDrawing();

            ClearBackground(RAYWHITE);

            rlTPCameraBeginMode3D(&orbitCam);

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) < 0) && time_since_move >= 0.2f) {
                  // FIXME only select live ones?
                  active_chess_piece = clamp(active_chess_piece - 1 % 16, 0, 15);
                  time_since_move = 0.0f;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) > 0.95) && time_since_move >= 0.2f) {
                  active_chess_piece = clamp(active_chess_piece + 1 % 16, 0, 15);
                  time_since_move = 0.0f;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) < 0) && time_since_move >= 0.2f) {
                  // FIXME only select live ones?
                  active_chess_piece = clamp(active_chess_piece - 8 % 16, 0, 15);
                  time_since_move = 0.0f;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) > 0.95f) && time_since_move >= 0.2f) {
                  active_chess_piece = clamp(active_chess_piece + 8 % 16, 0, 15);
                  time_since_move = 0.0f;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) > 0.95f) && time_since_move >= 0.2f) {
                  active_chess_piece = clamp(active_chess_piece + 8 % 16, 0, 15);
                  time_since_move = 0.0f;
                }

                // Camera controls
                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, RIGHT_STICK_LEFT_RIGHT) < 0.0f) && time_since_move >= 0.2f) {
                  time_since_move = 0.0f;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, RIGHT_STICK_LEFT_RIGHT) > 0.95f) && time_since_move >= 0.2f) {
                  time_since_move = 0.0f;
                }

                time_since_move += GetFrameTime();

                for (int i = 0; i < 16; i++) {
                  Vector3 gridPos = white_cells.grid_positions[i];
                  int pieceType = whitePieces.chess_type[i];
                  Model model = chessTypes.models[pieceType];
                  float scaling_factor = chessTypes.scaling_factors[pieceType];
                  DrawModel(model, gridPos, scaling_factor, WHITE);
                }

                for (int i = 0; i < 16; i++) {
                  Vector3 gridPos = black_cells.grid_positions[i];
                  int pieceType = blackPieces.chess_type[i];
                  Model model = chessTypes.models[pieceType];
                  float scaling_factor = chessTypes.scaling_factors[pieceType];
                  DrawModel(model, gridPos, scaling_factor, BLACK);
                }

                struct ChessPieces activePieces = players.pieces[active_player];
                int activePieceType = activePieces.chess_type[active_chess_piece];

                Vector2 *offsets = chessTypes.offsets[activePieceType];
                int offsetNum = chessTypes.offset_sizes[activePieceType];

                Vector3 highlight_pos = activePieces.cells.grid_positions[active_chess_piece];
                highlight_pos.y = 0; // Setting the height of it
                DrawCube(highlight_pos, 5, 0.1f, 5, RED);

                for (int offsetIndex = 0; offsetIndex < offsetNum; offsetIndex++) {
                  Vector2 offset = offsets[offsetIndex];
                  Vector2 active_chess_pos = activePieces.cells.chess_positions[active_chess_piece];

                  Vector2 move_chess_pos;
                  // This is kind of janky, might decide to store the transformed coordinates and just look them up?
                  move_chess_pos.x = convertCoord(convertCoord(active_chess_pos.x, 8) + offset.y, 8);
                  move_chess_pos.y = convertCoord(convertCoord(active_chess_pos.y, 8) + offset.x, 8);

                  Vector3 move_position = calculateMove(move_chess_pos.x, move_chess_pos.y, pieceSize);

                  DrawCube(move_position, 5, 0.1f, 5, GREEN);
                }

                DrawGrid(8, 5.0f);
            rlTPCameraEndMode3D();

            DrawRectangle( 10, 10, 320, 93, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines( 10, 10, 320, 93, BLUE);

            DrawText("Free camera default controls:", 20, 20, 10, BLACK);
            DrawText("- Mouse Wheel to Zoom in-out", 40, 40, 10, DARKGRAY);
            DrawText("- Mouse Wheel Pressed to Pan", 40, 60, 10, DARKGRAY);
            DrawText("- Z to zoom to (0, 0, 0)", 40, 80, 10, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
