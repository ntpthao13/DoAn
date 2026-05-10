#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOGDI
#define NOUSER

#include "raylib.h"
#include <windows.h>
#include <sqlext.h>

#include "Pathfinding.h"
#include "Product.h"
#include "InventoryManager.h"

using namespace std;

const Color COLOR_BG = { 10, 10, 10, 255 };
const Color COLOR_GRID_LINES = { 30, 30, 30, 255 };
const Color COLOR_OBSTACLE = { 200, 200, 200, 255 };
const Color COLOR_PATH_LINE = { 0, 180, 216, 255 };
const Color COLOR_UI_BORDER = { 60, 60, 60, 255 };
const Color COLOR_PANEL = { 20, 20, 25, 255 };
const Color COLOR_TEXT_DIM = { 120, 120, 120, 255 };
const Color COLOR_UI_ACCENT = { 0, 200, 80, 255 };

float GetRotationTime(float startAngle, float targetAngle, float rotSpeed) {
    float angleDiff = targetAngle - startAngle;
    while (angleDiff > PI) angleDiff -= 2 * PI;
    while (angleDiff < -PI) angleDiff += 2 * PI;
    return (abs(angleDiff) / rotSpeed) / 60.0f;
}

float GetPathRotationTime(const vector<Node*>& path, float initialAngle, float rotSpeed) {
    if (path.size() < 2) return 0.0f;
    float totalRotTime = 0.0f;
    float currentAngle = initialAngle;
    for (size_t i = 1; i < path.size(); i++) {
        float targetAngle = atan2f((float)path[i]->row - (float)path[i - 1]->row, (float)path[i]->col - (float)path[i - 1]->col);
        totalRotTime += GetRotationTime(currentAngle, targetAngle, rotSpeed);
        currentAngle = targetAngle;
    }
    return totalRotTime;
}

vector<vector<int>> LoadMap(const string& fileName) {
    ifstream file(fileName);
    vector<vector<int>> grid;
    if (file.is_open()) {
        int rows, cols;
        file >> rows >> cols;
        grid.resize(rows, vector<int>(cols));
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) file >> grid[i][j];
        }
        file.close();
    }
    return grid;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(1280, 720, "WAREHOUSE AGV NAVIGATION SYSTEM");
    SetTargetFPS(60);

    vector<vector<int>> grid = LoadMap("D:/Thao/DoAn/map.txt");
    if (grid.empty()) grid = vector<vector<int>>(20, vector<int>(30, 0));
    int rows = grid.size(), cols = grid[0].size();
    vector<vector<int>> staticGrid = grid;

    vector<Product> inventory = InventoryManager::InitializeInventory();
    int selectedProductIdx = -1;
    bool dropdownOpen = false;

    vector<double> weights = { 1.0, 1.2, 1.5, 2.0, 5.0 };
    int selectedWeightIdx = 0;
    bool dropdownWeightOpen = false;

    const int deliveryRow = 4, deliveryCol = 9;
    Font fontTitle = LoadFontEx("D:/Thao/DoAn/assets/fonts/Orbitron-Bold.ttf", 48, 0, 0);

    int robotRow = deliveryRow, robotCol = deliveryCol;
    float visualAngle = 0.0f, rotationSpeed = 0.05f;

    vector<pair<int, int>> fullPath;
    vector<Node*> pathNodes, returnPathNodes;

    int pathIndex = 0, systemState = 0;
    float moveTimer = 0, moveSpeed = 0.4f, pickupTimer = 0.0f;
    bool isWaitingAtPickup = false;
    string statusText = "READY: SELECT PRODUCT";

    int pathLength = 0, nodesVisited = 0;
    float totalTravelTime = 0.0f, estimatedTime = 0.0f;

    while (!WindowShouldClose()) {
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float padding = 20.0f, rightPanelWidth = 380.0f, bottomPanelHeight = 160.0f, headerHeight = 70.0f;
        Rectangle gridArea = { padding, headerHeight + padding, sw - rightPanelWidth - (padding * 3), sh - headerHeight - bottomPanelHeight - (padding * 3) };
        float cellSize = fminf(gridArea.width / cols, gridArea.height / rows);
        float offsetX = gridArea.x + (gridArea.width - (cols * cellSize)) / 2;
        float offsetY = gridArea.y + (gridArea.height - (rows * cellSize)) / 2;

        Rectangle orderPanel = { gridArea.x + gridArea.width + padding, gridArea.y, rightPanelWidth, gridArea.height };
        Rectangle dropdownBox = { orderPanel.x + 20, orderPanel.y + 150, orderPanel.width - 40, 45 };
        Rectangle dropdownWeightBox = { orderPanel.x + 20, orderPanel.y + 210, orderPanel.width - 40, 45 };
        Rectangle perfPanel = { padding, gridArea.y + gridArea.height + padding, gridArea.width + padding + rightPanelWidth, bottomPanelHeight };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            bool actionTaken = false;
            if (dropdownWeightOpen) {
                for (int i = 0; i < (int)weights.size(); i++) {
                    Rectangle itemRect = { dropdownWeightBox.x, dropdownWeightBox.y + (float)(i + 1) * 45.0f, dropdownWeightBox.width, 45.0f };
                    if (CheckCollisionPointRec(mousePos, itemRect)) {
                        selectedWeightIdx = i; dropdownWeightOpen = false; actionTaken = true; break;
                    }
                }
                if (!actionTaken) dropdownWeightOpen = false;
                actionTaken = true;
            }
            else if (dropdownOpen) {
                for (int i = 0; i < (int)inventory.size(); i++) {
                    Rectangle itemRect = { dropdownBox.x, dropdownBox.y + (float)(i + 1) * 45.0f, dropdownBox.width, 45.0f };
                    if (CheckCollisionPointRec(mousePos, itemRect)) {
                        selectedProductIdx = i;
                        int sRow = inventory[i].shelfRow, sCol = inventory[i].shelfCol;
                        int originalVal = grid[sRow][sCol];
                        grid[sRow][sCol] = 0;
                        Pathfinding pGo, pBack;
                        double currentW = weights[selectedWeightIdx];
                        pathNodes = pGo.findPath(robotRow, robotCol, sRow, sCol, grid, currentW);

                        if (!pathNodes.empty() && pathNodes.size() >= 2) {
                            Node* targetNode = pathNodes[pathNodes.size() - 2];
                            Node* shelfNode = pathNodes.back();
                            returnPathNodes = pBack.findPath(targetNode->row, targetNode->col, deliveryRow, deliveryCol, grid, currentW);

                            if (!returnPathNodes.empty()) {
                                fullPath.clear();
                                for (size_t j = 0; j < pathNodes.size() - 1; j++) fullPath.push_back({ pathNodes[j]->row, pathNodes[j]->col });
                                inventory[i].targetRow = targetNode->row;
                                inventory[i].targetCol = targetNode->col;
                                pathLength = (int)pathNodes.size() - 2;
                                nodesVisited = pGo.getNodesVisited();
                                totalTravelTime = 0.0f; pathIndex = 1; systemState = 2;
                                statusText = "FETCHING: " + inventory[i].name;
                            }
                            else statusText = "ERROR: NO RETURN PATH!";
                        }
                        else statusText = "ERROR: PATH BLOCKED!";

                        grid[sRow][sCol] = originalVal; dropdownOpen = false; actionTaken = true; break;
                    }
                }
                if (!actionTaken) dropdownOpen = false;
                actionTaken = true;
            }
            if (!actionTaken && systemState == 0) {
                if (CheckCollisionPointRec(mousePos, dropdownBox)) { dropdownOpen = true; dropdownWeightOpen = false; }
                else if (CheckCollisionPointRec(mousePos, dropdownWeightBox)) { dropdownWeightOpen = true; dropdownOpen = false; }
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, { offsetX, offsetY, cols * cellSize, rows * cellSize })) {
                int c = (int)((mousePos.x - offsetX) / cellSize), r = (int)((mousePos.y - offsetY) / cellSize);
                if (r >= 0 && r < rows && c >= 0 && c < cols && !((r == robotRow && c == robotCol) || (r == deliveryRow && c == deliveryCol)))
                    grid[r][c] = (grid[r][c] == 0) ? 1 : 0;
            }
        }

        if (systemState == 2 && !fullPath.empty()) {
            totalTravelTime += GetFrameTime();

            // --- LOGIC SỬA ĐỔI ESTIMATETIME (TÍNH CẢ THỜI GIAN XOAY) ---
            bool isDelivering = (statusText.find("DELIVERING") != string::npos);

            // 1. Tính thời gian cho quãng đường hiện tại (Move + Rotation)
            int remainingNodesCurrent = (int)fullPath.size() - pathIndex;
            float moveTimeCurrent = remainingNodesCurrent * moveSpeed;

            vector<Node*> currentRemainingNodes;
            // Node hiện tại robot đang đứng
            currentRemainingNodes.push_back(new Node(robotRow, robotCol));
            // Các node còn lại trong path
            for (int k = pathIndex; k < (int)fullPath.size(); k++) {
                currentRemainingNodes.push_back(new Node(fullPath[k].first, fullPath[k].second));
            }
            if (!isDelivering) {
                currentRemainingNodes.push_back(new Node(inventory[selectedProductIdx].shelfRow, inventory[selectedProductIdx].shelfCol));
            }
            float rotTimeCurrent = GetPathRotationTime(currentRemainingNodes, visualAngle, rotationSpeed);
            for (auto n : currentRemainingNodes) delete n;
            currentRemainingNodes.clear();

            if (!isDelivering && !isWaitingAtPickup) {
                // Đang đi lấy hàng: Path hiện tại + Chờ + Path về
                float moveTimeReturn = (returnPathNodes.empty()) ? 0.0f : ((int)returnPathNodes.size() - 1) * moveSpeed;

                // Góc bắt đầu của path về là góc cuối của path đi
                float lastAngleGo = atan2f((float)inventory[selectedProductIdx].shelfRow - (float)inventory[selectedProductIdx].targetRow,
                    (float)inventory[selectedProductIdx].shelfCol - (float)inventory[selectedProductIdx].targetCol);
                float rotTimeReturn = GetPathRotationTime(returnPathNodes, lastAngleGo, rotationSpeed);

                estimatedTime = (moveTimeCurrent + rotTimeCurrent) + 2.0f + (moveTimeReturn + rotTimeReturn);
            }
            else if (isWaitingAtPickup) {
                // Đang chờ: Thời gian chờ còn lại + Path về
                float moveTimeReturn = (returnPathNodes.empty()) ? 0.0f : ((int)returnPathNodes.size() - 1) * moveSpeed;
                float angleLookingAtShelf = atan2f((float)inventory[selectedProductIdx].shelfRow - (float)robotRow,
                    (float)inventory[selectedProductIdx].shelfCol - (float)robotCol);
                float rotTimeReturn = GetPathRotationTime(returnPathNodes, angleLookingAtShelf, rotationSpeed);

                estimatedTime = rotTimeCurrent + (2.0f - pickupTimer) + (moveTimeReturn + rotTimeReturn);
            }
            else {
                // Đang giao hàng: Chỉ tính path hiện tại
                estimatedTime = moveTimeCurrent + rotTimeCurrent;
            }
            // ---------------------------------------------------------

            float targetX = isWaitingAtPickup ? (float)inventory[selectedProductIdx].shelfCol : (float)fullPath[pathIndex].second;
            float targetY = isWaitingAtPickup ? (float)inventory[selectedProductIdx].shelfRow : (float)fullPath[pathIndex].first;
            float targetAngle = atan2f(targetY - (float)robotRow, targetX - (float)robotCol);
            float angleDiff = targetAngle - visualAngle;
            while (angleDiff > PI) angleDiff -= 2 * PI;
            while (angleDiff < -PI) angleDiff += 2 * PI;

            if (abs(angleDiff) > rotationSpeed) visualAngle += (angleDiff > 0) ? rotationSpeed : -rotationSpeed;
            else {
                visualAngle = targetAngle;
                if (isWaitingAtPickup) {
                    statusText = "PICKUP: " + inventory[selectedProductIdx].name;
                    pickupTimer += GetFrameTime();
                    if (pickupTimer >= 2.0f) {
                        isWaitingAtPickup = false; pickupTimer = 0.0f; fullPath.clear();
                        for (auto n : returnPathNodes) fullPath.push_back({ n->row, n->col });
                        if (!fullPath.empty()) { pathIndex = 1; statusText = "DELIVERING TO STATION"; }
                        else { statusText = "ERROR: RETURN BLOCKED!"; systemState = 0; }
                    }
                }
                else {
                    moveTimer += GetFrameTime();
                    if (moveTimer >= moveSpeed) {
                        bool isBlocked = false;
                        for (int k = pathIndex; k < (int)fullPath.size(); k++) if (grid[fullPath[k].first][fullPath[k].second] == 1) { isBlocked = true; break; }

                        if (isBlocked) {
                            statusText = "RE-ROUTING...";
                            Pathfinding pRe;
                            double currentW = weights[selectedWeightIdx];

                            if (!isDelivering) {
                                int sRow = inventory[selectedProductIdx].shelfRow;
                                int sCol = inventory[selectedProductIdx].shelfCol;

                                int originalVal = grid[sRow][sCol];
                                grid[sRow][sCol] = 0;
                                vector<Node*> rePath = pRe.findPath(robotRow, robotCol, sRow, sCol, grid, currentW);
                                grid[sRow][sCol] = originalVal;

                                if (!rePath.empty() && rePath.size() >= 2) {
                                    Node* newTarget = rePath[rePath.size() - 2];
                                    inventory[selectedProductIdx].targetRow = newTarget->row;
                                    inventory[selectedProductIdx].targetCol = newTarget->col;

                                    fullPath.clear();
                                    for (size_t j = 0; j < rePath.size() - 1; j++) {
                                        fullPath.push_back({ rePath[j]->row, rePath[j]->col });
                                    }

                                    pathIndex = 1;
                                    pathLength = (int)fullPath.size() - 1;

                                    nodesVisited = pRe.getNodesVisited();
                                    statusText = "FETCHING: " + inventory[selectedProductIdx].name;

                                    Pathfinding pReturnRe;
                                    returnPathNodes = pReturnRe.findPath(newTarget->row, newTarget->col, deliveryRow, deliveryCol, grid, currentW);
                                }
                                else {
                                    statusText = "SYSTEM HALTED: NO PATH";
                                    systemState = 0;
                                    fullPath.clear();
                                }
                            }
                            else {
                                vector<Node*> rePath = pRe.findPath(robotRow, robotCol, deliveryRow, deliveryCol, grid, currentW);
                                if (!rePath.empty()) {
                                    fullPath.clear();
                                    for (auto n : rePath) fullPath.push_back({ n->row, n->col });
                                    pathIndex = 1;
                                    pathLength = (int)rePath.size() - 1;

                                    nodesVisited = pRe.getNodesVisited();
                                    statusText = "DELIVERING TO STATION";
                                }
                                else {
                                    statusText = "SYSTEM HALTED: BLOCKED HOME";
                                    systemState = 0;
                                    fullPath.clear();
                                }
                            }
                        }
                        else {
                            robotRow = fullPath[pathIndex].first; robotCol = fullPath[pathIndex].second;
                            if (pathIndex < (int)fullPath.size() - 1) pathIndex++;
                            else if (selectedProductIdx != -1 && robotRow == inventory[selectedProductIdx].targetRow && robotCol == inventory[selectedProductIdx].targetCol && !isDelivering) isWaitingAtPickup = true;
                            else {
                                systemState = 0; statusText = "MISSION COMPLETE";
                                for (int i = 0; i < rows; i++) for (int j = 0; j < cols; j++) if (staticGrid[i][j] == 0) grid[i][j] = 0;
                                selectedProductIdx = -1; fullPath.clear(); estimatedTime = 0.0f;
                            }
                            moveTimer = 0.0f;
                        }
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(COLOR_BG);
        DrawTextEx(fontTitle, "WAREHOUSE AGV MONITORING", { 30, 25 }, 28, 2, WHITE);
        DrawRectangleLinesEx({ offsetX - 2, offsetY - 2, (cols * cellSize) + 4, (rows * cellSize) + 4 }, 2.0f, COLOR_UI_BORDER);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                float px = offsetX + (j * cellSize), py = offsetY + (i * cellSize);
                if (grid[i][j] == 1) DrawRectangleRec({ px, py, cellSize, cellSize }, COLOR_OBSTACLE);
                DrawRectangleLinesEx({ px, py, cellSize, cellSize }, 0.5f, COLOR_GRID_LINES);
                if (selectedProductIdx != -1 && i == inventory[selectedProductIdx].targetRow && j == inventory[selectedProductIdx].targetCol) DrawRectangleRec({ px + 2, py + 2, cellSize - 4, cellSize - 4 }, Fade(GREEN, 0.4f));
                if (i == deliveryRow && j == deliveryCol) DrawRectangleLinesEx({ px + 2, py + 2, cellSize - 4, cellSize - 4 }, 2.0f, RED);
            }
        }
        if (!fullPath.empty()) {
            for (int k = pathIndex; k < (int)fullPath.size() - 1; k++) {
                Vector2 s = { offsetX + (float)fullPath[k].second * cellSize + cellSize / 2, offsetY + (float)fullPath[k].first * cellSize + cellSize / 2 };
                Vector2 e = { offsetX + (float)fullPath[k + 1].second * cellSize + cellSize / 2, offsetY + (float)fullPath[k + 1].first * cellSize + cellSize / 2 };
                DrawLineEx(s, e, 3.0f, COLOR_PATH_LINE);
            }
        }
        float rx = offsetX + robotCol * cellSize + cellSize / 2, ry = offsetY + robotRow * cellSize + cellSize / 2;
        Rectangle rBody = { rx, ry, cellSize * 0.8f, cellSize * 0.6f };
        DrawRectanglePro(rBody, { rBody.width / 2, rBody.height / 2 }, visualAngle * RAD2DEG, COLOR_PATH_LINE);
        DrawCircleV({ rx + cosf(visualAngle) * (cellSize / 3), ry + sinf(visualAngle) * (cellSize / 3) }, cellSize / 8, WHITE);

        DrawRectangleRec(perfPanel, COLOR_PANEL); DrawRectangleLinesEx(perfPanel, 1.0f, COLOR_UI_BORDER);
        DrawTextEx(fontTitle, "ALGORITHM PERFORMANCE", { perfPanel.x + 25, perfPanel.y + 15 }, 18, 2, WHITE);
        float colWidth = (perfPanel.width - 50) / 3;
        DrawText(TextFormat("PATH: %d Nodes", pathLength), (int)perfPanel.x + 25, (int)perfPanel.y + 85, 22, COLOR_UI_ACCENT);
        DrawText(TextFormat("VISITED: %d Nodes", nodesVisited), (int)(perfPanel.x + 25 + colWidth), (int)perfPanel.y + 85, 22, COLOR_UI_ACCENT);
        DrawText(TextFormat("TIME: %.1f Sec", totalTravelTime), (int)(perfPanel.x + 25 + 2 * colWidth), (int)perfPanel.y + 85, 22, COLOR_UI_ACCENT);

        DrawRectangleRec(orderPanel, COLOR_PANEL); DrawRectangleLinesEx(orderPanel, 1.0f, COLOR_UI_BORDER);
        DrawTextEx(fontTitle, "LIVE ORDERS", { orderPanel.x + 25, orderPanel.y + 25 }, 20, 2, WHITE);
        DrawText(statusText.c_str(), (int)orderPanel.x + 25, (int)orderPanel.y + 80, 16, COLOR_UI_ACCENT);

        int displayTime = (estimatedTime > 0.1f) ? (int)ceil(estimatedTime) : 0;
        DrawText(TextFormat("ESTIMATED: %d s", displayTime), (int)orderPanel.x + 25, (int)orderPanel.y + 110, 16, YELLOW);

        DrawRectangleRec(dropdownBox, Color{ 40, 40, 50, 255 }); DrawRectangleLinesEx(dropdownBox, 1.0f, dropdownOpen ? COLOR_UI_ACCENT : COLOR_UI_BORDER);
        DrawText((selectedProductIdx == -1 ? "Select Product" : inventory[selectedProductIdx].name.c_str()), (int)dropdownBox.x + 10, (int)dropdownBox.y + 12, 18, WHITE);
        DrawRectangleRec(dropdownWeightBox, Color{ 40, 40, 50, 255 }); DrawRectangleLinesEx(dropdownWeightBox, 1.0f, dropdownWeightOpen ? COLOR_UI_ACCENT : COLOR_UI_BORDER);
        DrawText(TextFormat("Weight: %.1f", weights[selectedWeightIdx]), (int)dropdownWeightBox.x + 10, (int)dropdownWeightBox.y + 12, 18, WHITE);

        if (dropdownOpen) {
            for (int i = 0; i < (int)inventory.size(); i++) {
                Rectangle r = { dropdownBox.x, dropdownBox.y + (float)(i + 1) * 45.0f, dropdownBox.width, 45.0f };
                DrawRectangleRec(r, CheckCollisionPointRec(GetMousePosition(), r) ? COLOR_UI_ACCENT : Color{ 50, 50, 60, 255 });
                DrawText(inventory[i].name.c_str(), (int)r.x + 10, (int)r.y + 12, 18, WHITE);
            }
        }
        if (dropdownWeightOpen) {
            for (int i = 0; i < (int)weights.size(); i++) {
                Rectangle r = { dropdownWeightBox.x, dropdownWeightBox.y + (float)(i + 1) * 45.0f, dropdownWeightBox.width, 45.0f };
                DrawRectangleRec(r, CheckCollisionPointRec(GetMousePosition(), r) ? COLOR_UI_ACCENT : Color{ 50, 50, 60, 255 });
                DrawText(TextFormat("w = %.1f", weights[i]), (int)r.x + 10, (int)r.y + 12, 18, WHITE);
            }
        }
        EndDrawing();
    }
    UnloadFont(fontTitle); CloseWindow(); return 0;
}