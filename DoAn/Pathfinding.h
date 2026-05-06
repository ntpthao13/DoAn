#pragma once
#include "Node.h"
#include <vector>
#include<map>
using namespace std;
class Pathfinding {
public:
	int getNodesVisited() const { return nodesVisitedCount; }
	vector<Node*> findPath(int startRow, int startCol, int targetRow, int targetCol,
		const vector<vector<int>>& grid, double weight = 1.0);
	void clearNodes();
private:
	int nodesVisitedCount = 0;
	map<pair<int, int>, Node*> allNodes;
	bool isValid(int row, int col, int rows, int cols, const vector<vector<int>>& grid);
	vector<Node*> retracePath(Node* endNode);
};