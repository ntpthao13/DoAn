#pragma once
#include <string>

struct Product {
    int id;               
    std::string name;     
    int shelfRow;         
    int shelfCol;         

    int targetRow;
    int targetCol;

    std::string status;   
    int turnSide = 0;    
};