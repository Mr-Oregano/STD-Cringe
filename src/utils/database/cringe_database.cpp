#include "utils/database/cringe_database.h"
#include <stdexcept>
#include <fmt/format.h>

CringeDB::CringeDB(const std::string& dbFile) : db(nullptr), db_path(dbFile) {
    if (sqlite3_open(dbFile.c_str(), &db) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error(fmt::format("Failed to open database: {}", error));
    }

    // Enable foreign keys
    execute("PRAGMA foreign_keys = ON");
    
    // Set journal mode to WAL for better concurrency
    execute("PRAGMA journal_mode = WAL");
}

CringeDB::~CringeDB() {
    if (db != nullptr) {
        sqlite3_close(db);
    }
}

void CringeDB::execute(const std::string& sql, 
                      const std::vector<std::string>& params) const {
    sqlite3_stmt* raw_stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw_stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", 
                                           sqlite3_errmsg(db)));
    }

    Statement stmt(raw_stmt);

    // Bind parameters
    for (size_t i = 0; i < params.size(); i++) {
        if (sqlite3_bind_text(stmt.get(), i + 1, params[i].c_str(), -1, 
                            SQLITE_STATIC) != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to bind parameter {}: {}", 
                                               i + 1, sqlite3_errmsg(db)));
        }
    }

    // Execute the statement
    rc = sqlite3_step(stmt.get());
    
    // For INSERT/UPDATE/DELETE operations, SQLITE_DONE is success
    // For SELECT operations, both SQLITE_ROW and SQLITE_DONE are valid results
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", 
                                           sqlite3_errmsg(db)));
    }
}

auto CringeDB::query(
    const std::string& sql, const std::vector<std::string>& params) const -> std::vector<std::vector<std::string>> {
    sqlite3_stmt* raw_stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &raw_stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare query: {}", 
                                           sqlite3_errmsg(db)));
    }

    const Statement stmt(raw_stmt);

    // Bind parameters
    for (size_t i = 0; i < params.size(); i++) {
        if (sqlite3_bind_text(stmt.get(), i + 1, params[i].c_str(), -1, 
                            SQLITE_STATIC) != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to bind parameter {}: {}", 
                                               i + 1, sqlite3_errmsg(db)));
        }
    }

    std::vector<std::vector<std::string>> results;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::vector<std::string> row;
        for (int i = 0; i < sqlite3_column_count(stmt.get()); i++) {
            const unsigned char* text = sqlite3_column_text(stmt.get(), i);
            row.emplace_back((text != nullptr) ? reinterpret_cast<const char*>(text) : "");
        }
        results.push_back(std::move(row));
    }

    return results;
}

void CringeDB::beginTransaction() const {
    execute("BEGIN TRANSACTION");
}

void CringeDB::commit() const {
    execute("COMMIT");
}

void CringeDB::rollback() const {
    execute("ROLLBACK");
}

auto CringeDB::tableExists(const std::string& tableName) const -> bool {
    const auto result = query(
        "SELECT name FROM sqlite_master WHERE type='table' AND name=?",
        {tableName}
    );
    return !result.empty();
}

void CringeDB::createIndex(const std::string& indexName, 
                          const std::string& tableName,
                          const std::string& columns) const {
    execute(fmt::format("CREATE INDEX IF NOT EXISTS {} ON {}({})",
                       indexName, tableName, columns));
}