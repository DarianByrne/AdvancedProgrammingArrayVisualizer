// array_visualizer.cpp
// Single-file raylib program: Interactive Array Visualizer with UI buttons and text input
// Requires: raylib (and raymath.h) installed
// Compile example (Linux): g++ -std=c++17 -O2 -lraylib array_visualizer.cpp -o array_visualizer

#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <optional>

// ---------- Configurable constants ----------
static const int SCREEN_W = 1100;
static const int SCREEN_H = 650;

static const int UI_PANEL_H = 100;
static const float BOX_W = 72.0f;
static const float BOX_H = 72.0f;
static const float BOX_SPACING = 16.0f;
static const float ARRAY_START_X = 50.0f;
static const float ARRAY_Y = (SCREEN_H - UI_PANEL_H) / 2.0f - BOX_H * 0.5f;

static const float MOVE_SPEED = 6.0f; // higher -> faster interpolation
static const Color BACKGROUND = {22, 28, 35, 255};

// colors for states
static const Color COL_BOX = {40, 44, 52, 255};
static const Color COL_BORDER = {100, 110, 120, 255};
static const Color COL_ACTIVE = {235, 147, 64, 255};    // active/selected
static const Color COL_COMPARE = {66, 135, 245, 255};   // being compared
static const Color COL_SWAP = {245, 66, 66, 255};       // swap highlight
static const Color COL_TARGET = {77, 201, 116, 255};    // insertion target
static const Color COL_TEXT = WHITE;

// ---------- Utility ----------
struct Button {
    Rectangle rect;
    std::string label;
    bool enabled = true;

    bool DrawAndHandle()
    {
        Color bg = enabled ? Fade(RAYWHITE, 0.05f) : Fade(RAYWHITE, 0.02f);
        Vector2 m = GetMousePosition();
        bool hover = enabled && CheckCollisionPointRec(m, rect);
        bool pressed = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && enabled;

        // draw
        Color fill = enabled ? (hover ? LIGHTGRAY : bg) : DARKGRAY;
        DrawRectangleRec(rect, fill);
        DrawRectangleLinesEx(rect, 2.0f, COL_BORDER);
        int fontSize = 18;
        int textW = MeasureText(label.c_str(), fontSize);
        DrawText(label.c_str(), (int)(rect.x + rect.width * 0.5f - textW * 0.5f),
                 (int)(rect.y + rect.height * 0.5f - fontSize * 0.5f), fontSize, BLACK);

        return pressed;
    }
};

// ---------- Visual element ----------
struct VElement {
    int value;
    Vector2 pos;       // current on-screen pos
    Vector2 target;    // target pos to interpolate toward
    float scale = 1.0f;
    float alpha = 1.0f;
    Color color = COL_BOX;
    bool moving = false;

    VElement(int v = 0, Vector2 p = {0,0}) : value(v), pos(p), target(p) {}
};

// ---------- ArrayVisualizer ----------
class ArrayVisualizer {
public:
    ArrayVisualizer()
    {
        elements.reserve(64);
    }

    void Draw()
    {
        // draw elements
        for (size_t i = 0; i < elements.size(); ++i) {
            const VElement &e = elements[i];
            DrawElement(e, (int)i);
        }
    }

    void Update(float dt)
    {
        // move elements toward target positions
        bool anyMoving = false;
        for (auto &e : elements) {
            // interpolate position
            e.pos = Vector2Lerp(e.pos, e.target, std::min(1.0f, MOVE_SPEED * dt));
            // clamp small distances
            if (Vector2Distance(e.pos, e.target) > 0.5f) {
                anyMoving = true;
                e.moving = true;
            } else {
                e.pos = e.target;
                e.moving = false;
            }
            // simple interpolation for scale/alpha if needed (not heavy)
            e.scale = Lerp(e.scale, 1.0f, std::min(1.0f, MOVE_SPEED * dt));
            e.alpha = Lerp(e.alpha, 1.0f, std::min(1.0f, MOVE_SPEED * dt));
        }

        // If an animation step was triggered (e.g., compare / swap), check if finished then progress
        if (runningMode == Mode::SEARCHING) {
            if (!AnyElementMoving()) {
                // continue searching
                ContinueSearch();
            }
        } else if (runningMode == Mode::SORTING) {
            if (!AnyElementMoving()) {
                ContinueSort();
            }
        } else if (runningMode == Mode::SHIFTING) {
            if (!AnyElementMoving()) {
                // continue shifting or finalize insertion
                ContinueShifting();
            }
        } else if (runningMode == Mode::DELETING) {
            if (!AnyElementMoving()) {
                // finalize deletion
                finalizeDelete();
                runningMode = Mode::IDLE;
                unlockUI();
            }
        } else if (runningMode == Mode::INSERTING) {
            if (!AnyElementMoving()) {
                // insertion finished (element placed)
                runningMode = Mode::IDLE;
                unlockUI();
            }
        }
    }

    // UI-level getters
    size_t size() const { return elements.size(); }

    // Add initial elements
    void InitWith(const std::vector<int>& vals)
    {
        elements.clear();
        for (size_t i = 0; i < vals.size(); ++i) {
            Vector2 p = indexToPos((int)i);
            VElement v(vals[i], p);
            elements.push_back(v);
        }
        updateTargets();
    }

    // Insert value at index (if index > size -> push back)
    void RequestInsert(int value, int index)
    {
        if (running()) return; // ignore while animating
        index = std::clamp(index, 0, (int)elements.size());
        
        insertionTargetIndex = index;
        insertionValue = value;
        
        // If inserting at the end, no shifting needed
        if (index >= (int)elements.size()) {
            Vector2 newPos = indexToPos(index);
            Vector2 startPos = { newPos.x, newPos.y - 120.0f };
            VElement ve(value, startPos);
            ve.target = newPos;
            ve.scale = 0.7f;
            ve.color = COL_TARGET;
            elements.push_back(ve);
            runningMode = Mode::INSERTING;
        } else {
            // Start shifting from the last element
            shiftingIndex = (int)elements.size() - 1;
            runningMode = Mode::SHIFTING;
        }
        
        lockUI();
    }

    // Delete by index
    void RequestDeleteAt(int index)
    {
        if (running() || elements.empty()) return;
        if (index < 0 || index >= (int)elements.size()) return;
        // animate deletion: scale down & fade out then remove & shift
        // We'll mark one element to shrink and then in finalizeDelete we remove it and shift others
        deletingIndex = index;
        elements[index].scale = 1.0f;
        elements[index].color = COL_SWAP;
        // target unchanged; but animate scale to 0 and alpha to 0 over time by setting immediate targets via lambda inside Update.
        // We'll use a simplistic approach: set scale target to 0 by manually decaying scale in Update loop while in DELETING state.
        runningMode = Mode::DELETING;
        lockUI();

        // set others' targets now to shift later (they will start shifting after we remove the element)
    }

    // Search for value (linear)
    void RequestSearchValue(int value)
    {
        if (running()) return;
        searchValue = value;
        searchIndex = 0;
        // reset colors
        resetColors();
        runningMode = Mode::SEARCHING;
        lockUI();
    }

    // Start bubble sort (animated step-by-step)
    void RequestSort()
    {
        if (running()) return;
        runningMode = Mode::SORTING;
        sortI = 0;
        sortJ = 0;
        resetColors();
        lockUI();
    }

    // Small helper: returns whether animator is running
    bool running() const { return runningMode != Mode::IDLE; }

private:
    enum class Mode { IDLE, INSERTING, SHIFTING, DELETING, SEARCHING, SORTING };
    Mode runningMode = Mode::IDLE;

    std::vector<VElement> elements;

    // delete helpers
    int deletingIndex = -1;

    // search helpers
    int searchValue = 0;
    int searchIndex = 0;

    // sort helpers (bubble)
    int sortI = 0;
    int sortJ = 0;

    // insertion helpers
    int insertionTargetIndex = -1;
    int insertionValue = 0;
    int shiftingIndex = -1; // tracks which element is currently being shifted

    // UI lock callback (simple flags; UI code checks these)
    bool uiLocked = false;
    void lockUI() { uiLocked = true; }
    void unlockUI() { uiLocked = false; }
public:
    bool isUILocked() const { return uiLocked; }

private:
    // Update target positions based on current element order
    void updateTargets()
    {
        for (size_t i = 0; i < elements.size(); ++i) {
            elements[i].target = indexToPos((int)i);
        }
    }

    Vector2 indexToPos(int idx) const
    {
        float x = ARRAY_START_X + idx * (BOX_W + BOX_SPACING);
        float y = ARRAY_Y;
        return { x, y };
    }

    void DrawElement(const VElement &e, int idx) const
    {
        Vector2 pos = e.pos;
        float w = BOX_W * e.scale;
        float h = BOX_H * e.scale;
        Rectangle r = { pos.x - (BOX_W - w) * 0.5f, pos.y - (BOX_H - h) * 0.5f, w, h };
        Color col = e.color;
        col.a = (unsigned char)(e.alpha * 255.0f);

        DrawRectangleRounded(r, 0.12f, 6, col);
        DrawRectangleRoundedLinesEx(r, 0.12f, 6, 2.0f, COL_BORDER);

        // value text
        std::string s = std::to_string(e.value);
        int fs = 20;
        int tw = MeasureText(s.c_str(), fs);
        DrawText(s.c_str(), (int)(r.x + r.width * 0.5f - tw * 0.5f),
                 (int)(r.y + r.height * 0.5f - fs * 0.5f), fs, COL_TEXT);
    }

    bool AnyElementMoving() const
    {
        for (const auto &e : elements) if (e.moving) return true;
        return false;
    }

    void resetColors()
    {
        for (auto &e : elements) e.color = COL_BOX;
    }

    // Called each frame while SEARCHING and not moving to step to next index
    void ContinueSearch()
    {
        // if searchIndex >= size -> not found
        if (searchIndex >= (int)elements.size()) {
            // search finished
            // maybe flash "not found" by coloring none; here we just unlock UI
            runningMode = Mode::IDLE;
            unlockUI();
            return;
        }

        // highlight current element
        resetColors();
        elements[searchIndex].color = COL_ACTIVE;

        // if equal -> highlight target and stop
        if (elements[searchIndex].value == searchValue) {
            elements[searchIndex].color = COL_TARGET;
            runningMode = Mode::IDLE;
            unlockUI();
            return;
        } else {
            // animate a small hop or color change to indicate check
            // We'll animate a tiny vertical movement
            Vector2 orig = elements[searchIndex].target;
            elements[searchIndex].pos.y = orig.y - 18.0f; // quick jump up
            elements[searchIndex].target.y = orig.y;     // then return to target
            // increment index to check next once movement completes
            searchIndex++;
            // keep runningMode = SEARCHING
        }
    }

    // Called each frame while SORTING and not moving to perform next compare/swap step
    void ContinueSort()
    {
        int n = (int)elements.size();
        if (n < 2) {
            runningMode = Mode::IDLE;
            unlockUI();
            return;
        }

        if (sortI >= n - 1) {
            // finished
            resetColors();
            runningMode = Mode::IDLE;
            unlockUI();
            return;
        }
        if (sortJ >= n - 1 - sortI) {
            // advance i
            sortJ = 0;
            sortI++;
            // continue
            return;
        }

        // highlight pair being compared
        resetColors();
        elements[sortJ].color = COL_COMPARE;
        elements[sortJ + 1].color = COL_COMPARE;

        // small delay simulation: use a short upward movement then compare
        // We'll animate a slight vertical lift for the pair, then either swap or return.
        Vector2 aTarget = elements[sortJ].target;
        Vector2 bTarget = elements[sortJ + 1].target;

        // create lifted targets to show comparison (small vertical offset)
        elements[sortJ].pos.y = aTarget.y - 14.0f;
        elements[sortJ].target.y = aTarget.y;
        elements[sortJ+1].pos.y = bTarget.y - 14.0f;
        elements[sortJ+1].target.y = bTarget.y;

        // After movement settles, logic will re-enter ContinueSort() again
        // Decide swap immediately in state, then animate swap by exchanging target positions
        if (elements[sortJ].value > elements[sortJ + 1].value) {
            // swap the elements in the container but animate positions by swapping targets
            // To get a smooth swap, we swap the target positions and then swap elements in vector after movement finishes.
            // Simpler approach: swap elements in vector now but set their targets accordingly.
            std::swap(elements[sortJ], elements[sortJ + 1]);
            // Now fix their visual target positions to where they should be
            updateTargets();
            // mark colors
            elements[sortJ].color = COL_SWAP;
            elements[sortJ + 1].color = COL_SWAP;
        } else {
            // no swap; keep colors as compare and just advance indices
            // nothing else to animate; we already did a tiny lift-return
        }

        // advance j
        sortJ++;
    }

    // Called each frame while SHIFTING to shift elements one at a time
    void ContinueShifting()
    {
        // If we've shifted all elements down to the target index, insert the new element
        if (shiftingIndex < insertionTargetIndex) {
            // Insert the new element
            Vector2 newPos = indexToPos(insertionTargetIndex);
            Vector2 startPos = { newPos.x, newPos.y - 120.0f };
            VElement ve(insertionValue, startPos);
            ve.target = newPos;
            ve.scale = 0.7f;
            ve.color = COL_TARGET;
            elements.insert(elements.begin() + insertionTargetIndex, ve);
            
            runningMode = Mode::INSERTING;
            return;
        }
        
        // Shift the current element to the right
        elements[shiftingIndex].color = COL_ACTIVE;
        Vector2 newTarget = indexToPos(shiftingIndex + 1);
        elements[shiftingIndex].target = newTarget;
        
        // Move to the next element (going left)
        shiftingIndex--;
    }

    // finalize delete after shrink animation completed
    void finalizeDelete()
    {
        if (deletingIndex < 0 || deletingIndex >= (int)elements.size()) {
            deletingIndex = -1;
            return;
        }

        // remove element
        elements.erase(elements.begin() + deletingIndex);
        deletingIndex = -1;
        // update targets of remaining elements
        updateTargets();
    }

    // Linear interpolation helper
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
};

// ---------- Simple TextInput control (only integer support) ----------
struct IntTextInput {
    Rectangle rect;
    std::string content;
    bool active = false;
    std::string placeholder = "";

    IntTextInput() {}
    IntTextInput(Rectangle r, std::string ph = "") : rect(r), placeholder(ph) {}

    void Draw()
    {
        Color bg = active ? Fade(RAYWHITE, 0.07f) : Fade(RAYWHITE, 0.03f);
        DrawRectangleRec(rect, bg);
        DrawRectangleLinesEx(rect, 2.0f, COL_BORDER);

        std::string display = content.empty() ? placeholder : content;
        int fs = 20;
        int tw = MeasureText(display.c_str(), fs);
        Color col = content.empty() ? GRAY : BLACK;
        DrawText(display.c_str(), (int)(rect.x + 8), (int)(rect.y + rect.height * 0.5f - fs * 0.5f), fs, col);

        if (active) {
            // cursor
            int cx = (int)(rect.x + 8 + tw + 4);
            DrawLine(cx, (int)(rect.y + 8), cx, (int)(rect.y + rect.height - 8), BLACK);
        }
    }

    void HandleInput()
    {
        Vector2 m = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(m, rect)) {
                active = true;
                // Clear on click to ease input (optional)
                // content.clear();
            } else {
                active = false;
            }
        }

        if (!active) return;

        int key = GetCharPressed();
        while (key > 0) {
            // only allow digits and minus sign at first position
            if ((key >= '0' && key <= '9') || (key == '-' && content.empty())) {
                content.push_back((char)key);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (!content.empty()) content.pop_back();
        }
        // optional: allow Enter to deactivate
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            active = false;
        }
    }

    std::optional<int> GetValue() const
    {
        if (content.empty()) return std::nullopt;
        try {
            int v = std::stoi(content);
            return v;
        } catch (...) {
            return std::nullopt;
        }
    }

    void SetText(const std::string &s) { content = s; }
    void Clear() { content.clear(); }
};

// ---------- Main ----------
int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "Array Visualizer - raylib");
    SetTargetFPS(60);

    // load fonts if desired; we'll use default for brevity

    ArrayVisualizer viz;

    // initialize with some sample data
    std::vector<int> initial = {10, 4, 7, 2, 9, 11};
    viz.InitWith(initial);

    // UI elements
    Button btnInsert{ {880, 20, 180, 36}, "Insert" };
    Button btnDelete{ {880, 20 + 44, 180, 36}, "Delete" };
    Button btnSearch{ {880, 20 + 88, 180, 36}, "Search" };
    Button btnSort{ {880, 20 + 132, 180, 36}, "Sort (Bubble)" };

    IntTextInput inputValue({700, 20, 160, 36}, "value");
    IntTextInput inputIndex({700, 20 + 44, 160, 36}, "index");

    std::string infoMsg = "Click an operation. UI locked while animations run.";

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Handle inputs
        inputValue.HandleInput();
        inputIndex.HandleInput();

        // Draw
        BeginDrawing();
        ClearBackground(BACKGROUND);

        // UI panel background
        DrawRectangle(0, 0, SCREEN_W, UI_PANEL_H, Fade(RAYWHITE, 0.03f));
        DrawLine(0, UI_PANEL_H, SCREEN_W, UI_PANEL_H, Fade(COL_BORDER, 0.6f));

        // draw UI elements
        inputValue.Draw();
        inputIndex.Draw();

        // disable buttons when viz is locked
        bool locked = viz.isUILocked();
        btnInsert.enabled = !locked;
        btnDelete.enabled = !locked;
        btnSearch.enabled = !locked;
        btnSort.enabled = !locked;

        if (btnInsert.DrawAndHandle()) {
            // get values
            auto vOpt = inputValue.GetValue();
            auto iOpt = inputIndex.GetValue();
            if (!vOpt.has_value()) {
                infoMsg = "Provide integer value to insert.";
            } else {
                int val = vOpt.value();
                int idx = iOpt.has_value() ? iOpt.value() : (int)viz.size(); // default append
                viz.RequestInsert(val, idx);
                infoMsg = "Inserting " + std::to_string(val) + " at index " + std::to_string(idx) + "...";
                // optionally clear input
                // inputValue.Clear();
                // inputIndex.Clear();
            }
        }
        if (btnDelete.DrawAndHandle()) {
            auto iOpt = inputIndex.GetValue();
            if (!iOpt.has_value()) {
                infoMsg = "Provide an index to delete.";
            } else {
                int idx = iOpt.value();
                if (idx < 0 || idx >= (int)viz.size()) {
                    infoMsg = "Index out of range.";
                } else {
                    viz.RequestDeleteAt(idx);
                    infoMsg = "Deleting element at index " + std::to_string(idx) + "...";
                }
            }
        }
        if (btnSearch.DrawAndHandle()) {
            auto vOpt = inputValue.GetValue();
            if (!vOpt.has_value()) {
                infoMsg = "Provide integer value to search.";
            } else {
                viz.RequestSearchValue(vOpt.value());
                infoMsg = "Searching for " + std::to_string(vOpt.value()) + "...";
            }
        }
        if (btnSort.DrawAndHandle()) {
            viz.RequestSort();
            infoMsg = "Starting bubble sort...";
        }

        // draw info
        DrawText(infoMsg.c_str(), 16, 14, 18, LIGHTGRAY);

        // draw array
        viz.Update(dt);
        viz.Draw();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
