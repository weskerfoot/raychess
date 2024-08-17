#include "raylib.h"
#include "math.h"
#include "stdio.h"

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

const int NINTENDO_CONTROLLER = 1;

struct ChessPieces {
  Vector3 *positions;
  char is_dead[16];
};

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
      Vector3 position = { (size*i) - (size/2.0),
                           1.5f,
                           (size*j) - (size/2.0)}; // 4 = half the grid from center
      // there are always 16 pieces per player
      if (piece >= start && piece < end) { // there must be a nicer way of checking this
        pieces.positions[piece % 16] = position;
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


static Vector3 whitePositions[16];
static Vector3 blackPositions[16];

int
main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera free");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    Texture2D pawn = LoadTexture("resources/pawn.png");
    DisableCursor();

    SetTargetFPS(60);
    float cubeSize = 5.0f;

    struct ChessPieces whitePieces = {.positions = &whitePositions[0]};
    struct ChessPieces blackPieces = {.positions = &blackPositions[0]};

    setPieces(whitePieces, cubeSize, TOP_SIDE);
    setPieces(blackPieces, cubeSize, BOTTOM_SIDE);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        if (IsKeyPressed('Z')) camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                //printf("gamepad axis count = %d\n", GetGamepadAxisCount(NINTENDO_CONTROLLER));
                //printf("gamepad name = %s\n", GetGamepadName(NINTENDO_CONTROLLER));

                int axis_count = GetGamepadAxisCount(NINTENDO_CONTROLLER);

                //for (int axis = 0; axis < axis_count; axis++) {
                  //printf("axis %d = %f\n", axis, GetGamepadAxisMovement(NINTENDO_CONTROLLER, axis));
                //}

                if (GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) > 0.9) {
                  camera.position.y = clamp(camera.position.y - 1, -100, 100);
                }

                if (GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_UP_DOWN) < 0) {
                  camera.position.y = clamp(camera.position.y + 1, -100, 100);
                }

                if (GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) > 0.9) {
                  camera.position.x = clamp(camera.position.x - 1, 0, 100);
                }

                if (GetGamepadAxisMovement(NINTENDO_CONTROLLER, LEFT_STICK_LEFT_RIGHT) < 0) {
                  camera.position.x = clamp(camera.position.x + 1, 0, 100);
                }

                for (int i = 0; i < 16; i++) {
                  Vector3 cubePosition = whitePieces.positions[i];
                  DrawBillboard(camera, pawn, cubePosition, 2.0f, WHITE);
                }

                for (int i = 0; i < 16; i++) {
                  Vector3 cubePosition = blackPieces.positions[i];
                  DrawBillboard(camera, pawn, cubePosition, 2.0f, BLACK);
                }


                DrawGrid(8, 5.0f);
            EndMode3D();

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
