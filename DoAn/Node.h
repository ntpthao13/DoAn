#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include<queue>
using namespace std;
struct Node {
    int row, col;          
    double gCost;      
    double hCost;      
    double fCost;      
    Node* parent;      

    Node(int row, int col) : row(row), col(col), gCost(0), hCost(0), fCost(0), parent(nullptr) {}

    void calculateHeuristic(int targetRow, int targetCol, double weight = 1.0) {
        // 1. Tính kho?ng cách c? b?n (Manhattan)
        double baseH = abs(col - targetCol) + abs(row - targetRow);

        // 2. hCost ph?i là giá tr? ?ã có tr?ng s?
        this->hCost = baseH * weight;

        // 3. fCost ??n gi?n là t?ng c?a g và h
        this->fCost = (double)this->gCost + this->hCost;
    }
};
struct CompareNode {
    bool operator()(Node* const& n1, Node* const& n2) {
        return n1->fCost > n2->fCost;
    }
};
typedef priority_queue<Node*, vector<Node*>, CompareNode> OpenList;