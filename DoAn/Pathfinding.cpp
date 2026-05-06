#include "Pathfinding.h"
#include <map>
using namespace std;
vector<Node*> Pathfinding::findPath(int startRow, int startCol, int targetRow, int targetCol,
	const vector<vector<int>>& grid, double weight) {

	clearNodes(); // Hàm này s? reset nodesVisitedCount v? 0

	int rows = grid.size();
	int cols = grid[0].size();
	OpenList openList;
	map<pair<int, int>, bool> closedList;

	Node* startNode = new Node(startRow, startCol);
	startNode->calculateHeuristic(targetRow, targetCol, weight);
	openList.push(startNode);
	allNodes[{startRow, startCol}] = startNode;

	int dCol[] = { -1, 1, 0, 0 };
	int dRow[] = { 0, 0, -1, 1 };

	while (!openList.empty()) {
		Node* current = openList.top();
		openList.pop();

		// N?U CH?A XÉT THÌ T?NG BI?N ??M
		if (closedList[{current->row, current->col}]) continue;

		closedList[{current->row, current->col}] = true;
		nodesVisitedCount++; // ?ÂY CHÍNH LÀ S? NODE ?Ã XÉT

		if (current->row == targetRow && current->col == targetCol) {
			return retracePath(current);
		}

		for (int i = 0; i < 4; i++) {
			int nextCol = current->col + dCol[i];
			int nextRow = current->row + dRow[i];

			if (isValid(nextRow, nextCol, rows, cols, grid)) {
				if (closedList[{nextRow, nextCol}]) continue;

				double newGCost = current->gCost + 1;
				auto it = allNodes.find({ nextRow, nextCol });
				bool inOpenList = (it != allNodes.end());

				if (!inOpenList || newGCost < it->second->gCost) {
					Node* nextNode;
					if (!inOpenList) {
						nextNode = new Node(nextRow, nextCol);
						allNodes[{nextRow, nextCol}] = nextNode;
					}
					else {
						nextNode = it->second;
					}

					nextNode->parent = current;
					nextNode->gCost = newGCost;
					nextNode->calculateHeuristic(targetRow, targetCol, weight);
					openList.push(nextNode);
				}
			}
		}
	}
	return {};
}

bool Pathfinding::isValid(int row, int col, int rows, int cols, const vector<vector<int>>& grid) {
	return (row >= 0 && row < rows && col >= 0 && col < cols && grid[row][col] == 0);
}

vector<Node*> Pathfinding::retracePath(Node* endNode) {
	vector<Node*> path;
	Node* temp = endNode;
	while (temp != nullptr) {
		path.push_back(temp);
		temp = temp->parent;
	}
	reverse(path.begin(), path.end());
	return path;
}
void Pathfinding::clearNodes() {
	nodesVisitedCount = 0; // RESET L?I M?I KHI TÌM ???NG M?I
	for (auto& pair : allNodes) {
		if (pair.second != nullptr) {
			delete pair.second;
			pair.second = nullptr;
		}
	}
	allNodes.clear();
}