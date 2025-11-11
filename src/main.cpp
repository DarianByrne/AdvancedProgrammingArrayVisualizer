#include "raylib.h"
#include <string>
#include <sstream>
#include <cstdlib>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    const int arraySize = 10;

    InitWindow(screenWidth, screenHeight, " Array Visualizer");
    SetTargetFPS(60);

    // Array and UI variables
    int array[arraySize] = { 0 };
    int currentValue = 0;
    int selectedIndex = 0;

    // Beautiful color palette
    Color arrayColors[arraySize] = {
        Color{255, 107, 107, 255},    // Coral Red
        Color{255, 206, 107, 255},    // Peach
        Color{255, 255, 107, 255},    // Lemon Yellow
        Color{177, 255, 107, 255},    // Lime Green
        Color{107, 255, 157, 255},    // Mint
        Color{107, 255, 255, 255},    // Sky Blue
        Color{107, 157, 255, 255},    // Light Blue
        Color{157, 107, 255, 255},    // Lavender
        Color{255, 107, 255, 255},    // Pink
        Color{255, 107, 157, 255}     // Rose
    };

    // UI element rectangles
    Rectangle valuePlusButton = { 300, 450, 40, 40 };
    Rectangle valueMinusButton = { 350, 450, 40, 40 };
    Rectangle indexPlusButton = { 300, 500, 40, 40 };
    Rectangle indexMinusButton = { 350, 500, 40, 40 };
    Rectangle setButton = { 420, 475, 200, 40 };

    while (!WindowShouldClose()) {
        // Handle mouse input
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();

            // Check button clicks
            if (CheckCollisionPointRec(mousePos, valuePlusButton)) {
                currentValue++;
            }
            else if (CheckCollisionPointRec(mousePos, valueMinusButton)) {
                currentValue--;
            }
            else if (CheckCollisionPointRec(mousePos, indexPlusButton)) {
                selectedIndex = (selectedIndex + 1) % arraySize;
            }
            else if (CheckCollisionPointRec(mousePos, indexMinusButton)) {
                selectedIndex = (selectedIndex - 1 + arraySize) % arraySize;
            }
            else if (CheckCollisionPointRec(mousePos, setButton)) {
                if (selectedIndex >= 0 && selectedIndex < arraySize) {
                    array[selectedIndex] = currentValue;
                }
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(Color{ 25, 25, 35, 255 }); // Dark blue-gray background

        // Draw array elements
        for (int i = 0; i < arraySize; i++) {
            int x = 100 + i * 60;
            int y = 200;
            int width = 50;
            int height = 50;

            // Highlight selected index
            Color borderColor = (i == selectedIndex) ? Color{ 255, 255, 0, 255 } : Color{ 255, 255, 255, 200 };
            float borderThickness = (i == selectedIndex) ? 3.0f : 1.0f;

           
            // Draw border
            DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)width, (float)height }, borderThickness, borderColor);

            // Draw index number
            DrawText(TextFormat("%d", i), x + 20, y - 30, 20, Color{ 200, 200, 200, 255 });

            // Draw value
            DrawText(TextFormat("%d", array[i]), x + 15, y + 15, 20, WHITE);
        }

        // Draw UI elements
        // Value controls
        bool valuePlusHovered = CheckCollisionPointRec(GetMousePosition(), valuePlusButton);
        bool valueMinusHovered = CheckCollisionPointRec(GetMousePosition(), valueMinusButton);

        DrawRectangleRec(valuePlusButton, valuePlusHovered ? Color{ 86, 214, 86, 255 } : Color{ 66, 194, 66, 255 });
        DrawRectangleRec(valueMinusButton, valueMinusHovered ? Color{ 214, 86, 86, 255 } : Color{ 194, 66, 66, 255 });
        DrawRectangleLinesEx(valuePlusButton, 2, valuePlusHovered ? Color{ 120, 240, 120, 255 } : Color{ 100, 220, 100, 255 });
        DrawRectangleLinesEx(valueMinusButton, 2, valueMinusHovered ? Color{ 240, 120, 120, 255 } : Color{ 220, 100, 100, 255 });

        DrawText("+", valuePlusButton.x + 15, valuePlusButton.y + 10, 20, WHITE);
        DrawText("-", valueMinusButton.x + 15, valueMinusButton.y + 10, 20, WHITE);

        // Index controls
        bool indexPlusHovered = CheckCollisionPointRec(GetMousePosition(), indexPlusButton);
        bool indexMinusHovered = CheckCollisionPointRec(GetMousePosition(), indexMinusButton);

        DrawRectangleRec(indexPlusButton, indexPlusHovered ? Color{ 86, 156, 214, 255 } : Color{ 66, 136, 194, 255 });
        DrawRectangleRec(indexMinusButton, indexMinusHovered ? Color{ 86, 156, 214, 255 } : Color{ 66, 136, 194, 255 });
        DrawRectangleLinesEx(indexPlusButton, 2, indexPlusHovered ? Color{ 120, 180, 240, 255 } : Color{ 100, 160, 220, 255 });
        DrawRectangleLinesEx(indexMinusButton, 2, indexMinusHovered ? Color{ 120, 180, 240, 255 } : Color{ 100, 160, 220, 255 });

        DrawText("+", indexPlusButton.x + 15, indexPlusButton.y + 10, 20, WHITE);
        DrawText("-", indexMinusButton.x + 15, indexMinusButton.y + 10, 20, WHITE);

        // Set button
        bool setButtonHovered = CheckCollisionPointRec(GetMousePosition(), setButton);
        DrawRectangleRec(setButton, setButtonHovered ? Color{ 214, 156, 86, 255 } : Color{ 194, 136, 66, 255 });
        DrawRectangleLinesEx(setButton, 2, setButtonHovered ? Color{ 240, 180, 120, 255 } : Color{ 220, 160, 100, 255 });

        // Labels and text
        DrawText("Value:", 220, 455, 20, Color{ 220, 220, 220, 255 });
        DrawText("Index:", 220, 505, 20, Color{ 220, 220, 220, 255 });

        DrawText(TextFormat("%d", currentValue), 400, 455, 20, WHITE);
        DrawText(TextFormat("%d", selectedIndex), 400, 505, 20, WHITE);

        DrawText("SET VALUE", setButton.x + 10, setButton.y + 12, 20, WHITE);

        // Instructions
        DrawText("Use +/- buttons to adjust value and index, then click SET VALUE", 150, 550, 18, Color{ 180, 180, 180, 255 });
        DrawText("Colorful Array Visualizer", 250, 50, 30, Color{ 220, 220, 255, 255 });

        EndDrawing();
    }

    CloseWindow();
    return 0;
}