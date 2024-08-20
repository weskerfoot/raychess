#include "raylib.h"
#include "math.h"
#include "stdio.h"
#include "chess.h"
#include "camera/rlTPCamera.h"

#define COORD_TO_INDEX(x, y, row_n) (x + (y * row_n))

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

static int startingPieces[16] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,           // Second row
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK // First row
};

static Texture2D pieceTextures[6]; // 6 piece types
static Model pieceModels[6]; // 6 piece types
static float pieceScalingFactors[6] = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f};

const int NINTENDO_CONTROLLER = 1;

int
getCoord(int input, int n) {
  // Translate coordinates
  int half = n / 2;
  return half - input;
}

struct Players {
  int *score;
  int *player_type;
};

struct ChessPieces {
  Vector3 *grid_positions;
  Vector2 *chess_positions;
  int *chess_type;
  char *is_dead;
};

struct ChessTypes {
  int *chess_type;
  Texture2D *textures;
  Model *models;
  float *scaling_factors;
};

static void
printVec2(Vector2 vec) {
  printf("x = %f, y = %f\n", vec.x, vec.y);
}

static void
printVec3(Vector3 vec) {
  printf("x = %f, y = %f, z = %f\n", vec.x, vec.y, vec.z);
}

static Vector3 grid_positions[64];

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
        pieces.grid_positions[piece % 16] = position; // get rid of this and just do a lookup now?
        pieces.chess_positions[piece % 16] = (Vector2){i, j};
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


static Vector3 whiteGridPositions[16];
static Vector3 blackGridPositions[16];
static Vector2 whiteChessPositions[16];
static Vector2 blackChessPositions[16];
static char whitePiecesDead[16];
static char blackPiecesDead[16];

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

    // Define the camera to look into our 3d world
    //Camera3D camera = { 0 };
    //camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    //camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    //camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    //camera.fovy = 45.0f;                                // Camera field-of-view Y
    //camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    // Load all models
    for (int i = 0; i < 6; i++) {
      switch (i) {
        case PAWN:
          pieceModels[PAWN] = LoadModel("resources/models/chess_pieces_models/pawn.glb");
          break;
        case KNIGHT:
          pieceModels[KNIGHT] = LoadModel("resources/models/chess_pieces_models/knight.glb");
          break;
        case BISHOP:
          pieceModels[BISHOP] = LoadModel("resources/models/chess_pieces_models/bishop.glb");
          break;
        case ROOK:
          pieceModels[ROOK] = LoadModel("resources/models/chess_pieces_models/rook.glb");
          break;
        case QUEEN:
          pieceModels[QUEEN] = LoadModel("resources/models/chess_pieces_models/queen.glb");
          break;
        case KING:
          pieceModels[KING] = LoadModel("resources/models/chess_pieces_models/king.glb");
          break;
      }
    }

    DisableCursor();

    SetTargetFPS(60);
    float pieceSize = 5.0f;

    struct ChessPieces whitePieces = {
      .grid_positions = &whiteGridPositions[0],
      .chess_positions = &whiteChessPositions[0],
      .is_dead = &whitePiecesDead[0],
      .chess_type = &startingPieces[0]
    };

    struct ChessPieces blackPieces = {
      .grid_positions = &blackGridPositions[0],
      .chess_positions = &blackChessPositions[0],
      .is_dead = &blackPiecesDead[0],
      .chess_type = &startingPieces[0]
    };

    int score[2] = {0, 0};
    int active_players[2] = {WHITE_PLAYER, BLACK_PLAYER};

    struct Players players = {
      .score = &score[0],
      .player_type = &active_players[0]
    };

    // Use to store data about different piece types
    struct ChessTypes chessTypes = {
      .chess_type = &startingPieces[0],
      .textures = &pieceTextures[0],
      .models = &pieceModels[0],
      .scaling_factors = &pieceScalingFactors[0]
    };

    setPieces(whitePieces, pieceSize, TOP_SIDE);
    setPieces(blackPieces, pieceSize, BOTTOM_SIDE);

    int active_chess_piece = 0;
    int active_player = BLACK_PLAYER;

    //printVec2(whitePieces.chess_positions[3]);

    // Need to have inverted movements for one side vs the other
    blackPieces.chess_positions[0].x = getCoord(2, 8);
    blackPieces.chess_positions[0].y = getCoord(0, 8);
    blackPieces.grid_positions[0] = calculateMove(blackPieces.chess_positions[0].x, blackPieces.chess_positions[0].y, pieceSize);

    float time_since_move = 0;

    //for (int i = 0; i < 64; i++) {
      //printVec3(grid_positions[i]);
    //}

    while (!WindowShouldClose()) {
        rlTPCameraUpdate(&orbitCam);

        BeginDrawing();



            ClearBackground(RAYWHITE);

            rlTPCameraBeginMode3D(&orbitCam);

                //printf("gamepad axis count = %d\n", GetGamepadAxisCount(NINTENDO_CONTROLLER));
                //printf("gamepad name = %s\n", GetGamepadName(NINTENDO_CONTROLLER));

                //int axis_count = GetGamepadAxisCount(NINTENDO_CONTROLLER);

                //for (int axis = 0; axis < axis_count; axis++) {
                  //printf("axis %d = %f\n", axis, GetGamepadAxisMovement(NINTENDO_CONTROLLER, axis));
                //}

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) < 0) && time_since_move >= 0.2) {
                  // FIXME only select live ones?
                  active_chess_piece = clamp(active_chess_piece - 1 % 16, 0, 15);
                  time_since_move = 0;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) > 0.95) && time_since_move >= 0.2) {
                  active_chess_piece = clamp(active_chess_piece + 1 % 16, 0, 15);
                  time_since_move = 0;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) < 0) && time_since_move >= 0.2) {
                  // FIXME only select live ones?
                  active_chess_piece = clamp(active_chess_piece - 8 % 16, 0, 15);
                  time_since_move = 0;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) > 0.95) && time_since_move >= 0.2) {
                  active_chess_piece = clamp(active_chess_piece + 8 % 16, 0, 15);
                  time_since_move = 0;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) > 0.95) && time_since_move >= 0.2) {
                  active_chess_piece = clamp(active_chess_piece + 8 % 16, 0, 15);
                  time_since_move = 0;
                }

                // Camera controls
                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, RIGHT_STICK_LEFT_RIGHT) < 0) && time_since_move >= 0.2) {
                  time_since_move = 0;
                }

                if ((GetGamepadAxisMovement(NINTENDO_CONTROLLER, RIGHT_STICK_LEFT_RIGHT) > 0.95) && time_since_move >= 0.2) {
                  time_since_move = 0;
                }

                time_since_move += GetFrameTime();

                for (int i = 0; i < 16; i++) {
                  Vector3 gridPos = whitePieces.grid_positions[i];
                  int pieceType = whitePieces.chess_type[i];
                  Model model = chessTypes.models[pieceType];
                  float scaling_factor = chessTypes.scaling_factors[pieceType];
                  DrawModel(model, gridPos, scaling_factor, WHITE);
                }

                for (int i = 0; i < 16; i++) {
                  Vector3 gridPos = blackPieces.grid_positions[i];
                  int pieceType = whitePieces.chess_type[i];
                  Model model = chessTypes.models[pieceType];
                  float scaling_factor = chessTypes.scaling_factors[pieceType];
                  DrawModel(model, gridPos, scaling_factor, BLACK);
                }

                if (active_player == WHITE_PLAYER) {
                  Vector3 highlight_pos = whitePieces.grid_positions[active_chess_piece];
                  highlight_pos.y = 0;
                  DrawCube(highlight_pos, 5, 0.1, 5, RED);
                }

                if (active_player == BLACK_PLAYER) {
                  Vector3 highlight_pos = blackPieces.grid_positions[active_chess_piece];
                  highlight_pos.y = 0;
                  DrawCube(highlight_pos, 5, 0.1, 5, RED);
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
