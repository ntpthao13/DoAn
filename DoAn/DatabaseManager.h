#pragma once
#include <windows.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include <vector>
#include "Product.h"

class DatabaseManager {
public:
    std::vector<Product> GetAllProducts() {
        std::vector<Product> productList;
        SQLHENV hEnv;
        SQLHDBC hDbc;
        SQLHSTMT hStmt;
        SQLRETURN ret;

        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
        SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

        // C?p nh?t Database name thành WarehouseAGV theo file SQL v?a t?o
        std::string connStr = "Driver={ODBC Driver 17 for SQL Server};Server=LAPTOP-814KGM2F\\SQLEXPRESS;Database=QuanLyKhoHang2;Trusted_Connection=yes;";

        ret = SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

        if (SQL_SUCCEEDED(ret)) {
            SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

            // S? d?ng JOIN 3 b?ng ?? l?y thông tin ??y ??
            std::string query =
                "SELECT p.ProductID, p.ProductName, s.RowIndex, s.ColIndex, s.TargetRow, s.TargetCol "
                "FROM Products p "
                "JOIN Inventory i ON p.ProductID = i.ProductID "
                "JOIN Shelves s ON i.ShelfID = s.ShelfID";

            if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    Product p;
                    char buf[256];
                    SQLLEN indicatorRow, indicatorCol;

                    // 1. L?y ProductID
                    SQLGetData(hStmt, 1, SQL_C_SLONG, &p.id, 0, NULL);

                    // 2. L?y ProductName
                    SQLGetData(hStmt, 2, SQL_C_CHAR, buf, sizeof(buf), NULL);
                    p.name = std::string(buf);

                    // 3. L?y t?a ?? k? (RowIndex, ColIndex)
                    SQLGetData(hStmt, 3, SQL_C_SLONG, &p.shelfRow, 0, NULL);
                    SQLGetData(hStmt, 4, SQL_C_SLONG, &p.shelfCol, 0, NULL);

                    // 4. L?y Target (Có th? NULL nên c?n dùng indicator)
                    SQLGetData(hStmt, 5, SQL_C_SLONG, &p.targetRow, 0, &indicatorRow);
                    if (indicatorRow == SQL_NULL_DATA) p.targetRow = -1;

                    SQLGetData(hStmt, 6, SQL_C_SLONG, &p.targetCol, 0, &indicatorCol);
                    if (indicatorCol == SQL_NULL_DATA) p.targetCol = -1;

                    p.turnSide = 0;
                    p.status = "READY";

                    productList.push_back(p);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            std::cerr << "Loi: Khong the ket noi den SQL Server WarehouseAGV!" << std::endl;
        }

        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

        return productList;
    }
};