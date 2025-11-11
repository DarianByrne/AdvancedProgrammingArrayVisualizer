#include "raylib.h"

int flag() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, " Raylib flag!");
    SetTargetFPS(60);

    const int FLAG_X = 200;
    const int FLAG_Y = 100;
    const int FLAG_WIDTH = 400;
    const int FLAG_HEIGHT = 100;

    // Flag
    Rectangle redBg = {FLAG_X, FLAG_Y, FLAG_WIDTH, FLAG_HEIGHT};

    // UI element rectangles

    while (!WindowShouldClose()) {
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw UI elements

        DrawRectangleRec(redBg, RED);

        DrawCircle(FLAG_X + FLAG_WIDTH / 2.0, FLAG_Y + FLAG_HEIGHT / 2.0, 50, WHITE);

        DrawTriangle(
            {FLAG_X, FLAG_Y},
            {FLAG_X, FLAG_Y + FLAG_HEIGHT / 2.0},
            {FLAG_X + FLAG_HEIGHT / 2.0, FLAG_Y},
            BLUE
        );

        // Labels and text
        const char *label = "Raylib!";
        int fontSize = 20;
        int textWidth = MeasureText(label, fontSize);

        int textX = FLAG_X + (FLAG_WIDTH - textWidth) / 2;
        int textY = FLAG_Y + (FLAG_HEIGHT - fontSize) / 2;
        DrawText(label, textX, textY, fontSize, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
