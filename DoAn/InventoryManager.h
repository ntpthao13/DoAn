#pragma once
#include <vector>
#include "Product.h"
#include "DatabaseManager.h"

class InventoryManager {
public:
    static std::vector<Product> InitializeInventory() {
        DatabaseManager db;
        return db.GetAllProducts();
    }
};