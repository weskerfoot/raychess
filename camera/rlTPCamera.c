/**********************************************************************************************
*
*   raylibExtras * Utilities and Shared Components for Raylib
*
*   TPOrbitCamera * Third Person Camera Example
*
*   LICENSE: MIT
*
*   Copyright (c) 2021 Jeffery Myers
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
**********************************************************************************************/

#include "stdio.h"
#include "rlTPCamera.h"
#include "raylib.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>


Vector3
rlTPCameraGetScreenToWorld(rlTPCamera* camera, Vector2 screenPosition, float worldZ)
{
    if (camera == NULL)
        return (Vector3){0};

    // Generate a ray from the screen position
    Ray ray = GetMouseRay(screenPosition, camera->ViewCamera);

    // Calculate the intersection of the ray with a plane at the specified world Z
    float t = (worldZ - ray.position.z) / ray.direction.z;

    // Compute the world-space coordinates
    Vector3 worldPosition = {
        ray.position.x + t * ray.direction.x,
        ray.position.y + t * ray.direction.y,
        worldZ
    };

    return worldPosition;
}

static void ResizeTPOrbitCameraView(rlTPCamera* camera)
{
    if (camera == NULL)
        return;

    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();

    camera->FOV.y = camera->ViewCamera.fovy;

    if (height != 0)
        camera->FOV.x = camera->FOV.y * (width / height);
}

void rlTPCameraInit(rlTPCamera* camera, float fovY, Vector3 position)
{
    if (camera == NULL)
        return;

    camera->ControlsKeys[0] = 'T';
    camera->ControlsKeys[1] = 'G';
    camera->ControlsKeys[2] = 'F';
    camera->ControlsKeys[3] = 'H';
    camera->ControlsKeys[4] = 'E';
    camera->ControlsKeys[5] = 'Q';
    camera->ControlsKeys[6] = KEY_LEFT;
    camera->ControlsKeys[7] = KEY_RIGHT;
    camera->ControlsKeys[8] = KEY_UP;
    camera->ControlsKeys[9] = KEY_DOWN;
    camera->ControlsKeys[10] = KEY_LEFT_SHIFT;

    camera->MoveSpeed = (Vector3){ 3,3,3 };
    camera->TurnSpeed = (Vector2){ 90,90 };

    camera->MouseSensitivity = 600;

    camera->MinimumViewY = -89.0f;
    camera->MaximumViewY = 0.0f;

    camera->Focused = IsWindowFocused();

    camera->CameraPullbackDistance = 5;

    camera->ViewAngles = (Vector2){ 0,0 };

    camera->CameraPosition = position;
    camera->FOV.y = fovY;

    camera->ViewCamera.target = position;
    camera->ViewCamera.position = Vector3Add(camera->ViewCamera.target, (Vector3) { 0, 0, camera->CameraPullbackDistance });
    camera->ViewCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera->ViewCamera.fovy = fovY;
    camera->ViewCamera.projection = CAMERA_PERSPECTIVE;

    camera->NearPlane = 0.01;
    camera->FarPlane = 1000.0;

    ResizeTPOrbitCameraView(camera);
    rlTPCameraUseMouse(camera, true, 1);
}

void rlTPCameraUseMouse(rlTPCamera* camera, bool useMouse, int button)
{
    if (camera == NULL)
        return;

    camera->UseMouse = useMouse;
    camera->UseMouseButton = button;

    bool showCursor = !useMouse || button >= 0;

    if (!showCursor && IsWindowFocused())
        DisableCursor();
    else if (showCursor && IsWindowFocused())
        EnableCursor();
}

Vector3 rlTPCameraGetPosition(rlTPCamera* camera)
{
    return camera->CameraPosition;
}

void rlTPCameraSetPosition(rlTPCamera* camera, Vector3 position)
{
    camera->CameraPosition = position;
}

RLAPI Ray rlTPCameraGetViewRay(rlTPCamera* camera)
{
    return (Ray) {camera->ViewCamera.position, Vector3Subtract(camera->ViewCamera.target, camera->ViewCamera.position)};
}

static float GetSpeedForAxis(rlTPCamera* camera, rlTPCameraControls axis, float speed)
{
    if (camera == NULL)
        return 0;

    int key = camera->ControlsKeys[axis];
    if (key == -1)
        return 0;

    float factor = 1.0f;

    if (IsKeyDown(camera->ControlsKeys[SPRINT]))
        factor = 2;

    // FIXME pass in the controller instead of hardcoding 1
    if ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_RIGHT_X) > 0.95f) && axis == TURN_RIGHT) {
      return speed * GetFrameTime() * factor;
    }

    if ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_RIGHT_X) < 0.0f) && axis == TURN_LEFT) {
      return speed * GetFrameTime() * factor;
    }

    if ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_RIGHT_Y) > 0.95f) && axis == TURN_DOWN) {
      return speed * GetFrameTime() * factor;
    }

    if ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_RIGHT_Y) < 0.0f) && axis == TURN_UP) {
      return speed * GetFrameTime() * factor;
    }

    if (IsGamepadButtonDown(1, GAMEPAD_BUTTON_LEFT_TRIGGER_1) && axis == MOVE_LEFT) {
      return speed * GetFrameTime() * factor;
    }

    if (IsGamepadButtonDown(1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) && axis == MOVE_RIGHT) {
      return speed * GetFrameTime() * factor;
    }

    if (IsKeyDown(camera->ControlsKeys[axis])) {
        return speed * GetFrameTime() * factor;
    }

    return 0.0f;
}

void rlTPCameraUpdate(rlTPCamera* camera)
{
    if (camera == NULL)
        return;

    if (IsWindowResized())
        ResizeTPOrbitCameraView(camera);

    bool showCursor = !camera->UseMouse || camera->UseMouseButton >= 0;

    if (IsWindowFocused() != camera->Focused && !showCursor)
    {
        camera->Focused = IsWindowFocused();
        if (camera->Focused)
        {
            DisableCursor();
        }
        else
        {
            EnableCursor();
        }
    }

    // Mouse movement detection
    Vector2 mousePositionDelta = GetMouseDelta();
    float mouseWheelMove = GetMouseWheelMove();

    // Keys input detection
    float direction[MOVE_DOWN + 1] = { -GetSpeedForAxis(camera, MOVE_FRONT, camera->MoveSpeed.z),
                                      -GetSpeedForAxis(camera, MOVE_BACK, camera->MoveSpeed.z),
                                      GetSpeedForAxis(camera, MOVE_RIGHT, camera->MoveSpeed.x),
                                      GetSpeedForAxis(camera, MOVE_LEFT, camera->MoveSpeed.x),
                                      GetSpeedForAxis(camera, MOVE_UP, camera->MoveSpeed.y),
                                      GetSpeedForAxis(camera, MOVE_DOWN, camera->MoveSpeed.y) };

    bool useMouse = camera->UseMouse && (camera->UseMouseButton < 0 || IsMouseButtonDown(camera->UseMouseButton));

    float turnRotation = GetSpeedForAxis(camera, TURN_RIGHT, camera->TurnSpeed.x) - GetSpeedForAxis(camera, TURN_LEFT, camera->TurnSpeed.x);
    float tiltRotation = GetSpeedForAxis(camera, TURN_UP, camera->TurnSpeed.y) - GetSpeedForAxis(camera, TURN_DOWN, camera->TurnSpeed.y);

    if (turnRotation != 0)
        camera->ViewAngles.x -= turnRotation * DEG2RAD;
    else if (useMouse && camera->Focused)
        camera->ViewAngles.x -= (mousePositionDelta.x / camera->MouseSensitivity);

    if (tiltRotation)
        camera->ViewAngles.y += tiltRotation * DEG2RAD;
    else if (useMouse && camera->Focused)
        camera->ViewAngles.y += (mousePositionDelta.y / -camera->MouseSensitivity);

    // Angle clamp
    if (camera->ViewAngles.y < camera->MinimumViewY * DEG2RAD)
        camera->ViewAngles.y = camera->MinimumViewY * DEG2RAD;
    else if (camera->ViewAngles.y > camera->MaximumViewY * DEG2RAD)
        camera->ViewAngles.y = camera->MaximumViewY * DEG2RAD;

    //movement in plane rotation space
    Vector3 moveVec = { 0,0,0 };
    moveVec.z = direction[MOVE_FRONT] - direction[MOVE_BACK];
    moveVec.x = direction[MOVE_RIGHT] - direction[MOVE_LEFT];

    // update zoom
    camera->CameraPullbackDistance += GetMouseWheelMove();
    if (camera->CameraPullbackDistance < 1)
        camera->CameraPullbackDistance = 1;

    // vector we are going to transform to get the camera offset from the target point
    Vector3 camPos = { 0, 0, camera->CameraPullbackDistance };

    Matrix tiltMat = MatrixRotateX(camera->ViewAngles.y); // a matrix for the tilt rotation
    Matrix rotMat = MatrixRotateY(camera->ViewAngles.x); // a matrix for the plane rotation
    Matrix mat = MatrixMultiply(tiltMat, rotMat); // the combined transformation matrix for the camera position

    camPos = Vector3Transform(camPos, mat); // transform the camera position into a vector in world space
    moveVec = Vector3Transform(moveVec, rotMat); // transform the movement vector into world space, but ignore the tilt so it is in plane

    camera->CameraPosition = Vector3Add(camera->CameraPosition, moveVec); // move the target to the moved position

    // validate cam pos here

    // set the view camera
    camera->ViewCamera.target = camera->CameraPosition;
    camera->ViewCamera.position = Vector3Add(camera->CameraPosition, camPos); // offset the camera position by the vector from the target position
}

static void SetupCamera(rlTPCamera* camera, float aspect)
{
    rlDrawRenderBatchActive();			// Draw Buffers (Only OpenGL 3+ and ES2)
    rlMatrixMode(RL_PROJECTION);        // Switch to projection matrix
    rlPushMatrix();                     // Save previous matrix, which contains the settings for the 2d ortho projection
    rlLoadIdentity();                   // Reset current matrix (projection)

    if (camera->ViewCamera.projection == CAMERA_PERSPECTIVE)
    {
        // Setup perspective projection
        double top = RL_CULL_DISTANCE_NEAR * tan(camera->ViewCamera.fovy * 0.5 * DEG2RAD);
        double right = top * aspect;

        rlFrustum(-right, right, -top, top, camera->NearPlane, camera->FarPlane);
    }
    else if (camera->ViewCamera.projection == CAMERA_ORTHOGRAPHIC)
    {
        // Setup orthographic projection
        double top = camera->ViewCamera.fovy / 2.0;
        double right = top * aspect;

        rlOrtho(-right, right, -top, top, camera->NearPlane, camera->FarPlane);
    }

    // NOTE: zNear and zFar values are important when computing depth buffer values

    rlMatrixMode(RL_MODELVIEW);         // Switch back to modelview matrix
    rlLoadIdentity();                   // Reset current matrix (modelview)

    // Setup Camera view
    Matrix matView = MatrixLookAt(camera->ViewCamera.position, camera->ViewCamera.target, camera->ViewCamera.up);
    rlMultMatrixf(MatrixToFloatV(matView).v);      // Multiply modelview matrix by view matrix (camera)

    rlEnableDepthTest();                // Enable DEPTH_TEST for 3D
}

// start drawing using the camera, with near/far plane support
void rlTPCameraBeginMode3D(rlTPCamera* camera)
{
    if (camera == NULL)
        return;

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    SetupCamera(camera, aspect);
}

// end drawing with the camera
void rlTPCameraEndMode3D()
{
    EndMode3D();
}
