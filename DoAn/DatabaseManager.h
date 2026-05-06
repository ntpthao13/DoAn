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

        std::string connStr = "Driver={ODBC Driver 17 for SQL Server};Server=LAPTOP-814KGM2F\\SQLEXPRESS;Database=QuanLyKhoHang;Trusted_Connection=yes;";

        ret = SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

        if (SQL_SUCCEEDED(ret)) {
            SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

            std::string query = "SELECT ProductName, ShelfRow, ShelfCol FROM Products";

            if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    Product p;
                    char buf[256];

                    SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
                    p.name = std::string(buf);

                    SQLGetData(hStmt, 2, SQL_C_SLONG, &p.shelfRow, 0, NULL);

                    SQLGetData(hStmt, 3, SQL_C_SLONG, &p.shelfCol, 0, NULL);

                    p.targetRow = -1;
                    p.targetCol = -1;
                    p.turnSide = 0;

                    productList.push_back(p);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        else {
            std::cerr << "Loi: Khong the ket noi den SQL Server QuanLyKhoHang!" << std::endl;
        }

        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

        return productList;
    }
};