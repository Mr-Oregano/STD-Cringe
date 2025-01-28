#ifndef CRINGE_DATABASE_H
#define CRINGE_DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <future>        // Add this for std::promise, std::future
#include <thread>        // Add this for std::thread
#include <chrono>       // Add this for timeouts
#include <functional>   // Add this for std::forward


class CringeDB {
private:
    sqlite3* db;
    std::string db_path;

    // Helper to ensure RAII for sqlite3_stmt
    class Statement {
    private:
        sqlite3_stmt* stmt;
    public:
        explicit Statement(sqlite3_stmt* statement) : stmt(statement) {}
        ~Statement() { if (stmt != nullptr) { sqlite3_finalize(stmt);
}}
        [[nodiscard]] auto get() const -> sqlite3_stmt* { return stmt; }
    };

public:
    explicit CringeDB(const std::string& dbFile);
    ~CringeDB();

    // Prevent copying
    CringeDB(const CringeDB&) = delete;
    auto operator=(const CringeDB&) -> CringeDB& = delete;

    // Basic operations
    void execute(const std::string& sql, const std::vector<std::string>& params = {}) const;
    [[nodiscard]] auto query(const std::string& sql, const std::vector<std::string>& params = {}) const -> std::vector<std::vector<std::string>>;

    // Transaction support
    void beginTransaction() const;
    void commit() const;
    void rollback() const;

    [[nodiscard]] auto tableExists(const std::string &tableName) const -> bool;

    // Utility methods
    void createIndex(const std::string& indexName, const std::string& tableName,
                    const std::string& columns) const;

    template<typename F>
    auto executeWithTimeout(F&& func, std::chrono::milliseconds timeout) -> decltype(func()) {
        std::promise<decltype(func())> promise;
        auto future = promise.get_future();
        
        std::thread worker([&promise, func = std::forward<F>(func)]() {
            try {
                if constexpr (std::is_void_v<decltype(func())>) {
                    func();
                    promise.set_value();
                } else {
                    promise.set_value(func());
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });
        
        worker.detach();
        
        if (future.wait_for(timeout) == std::future_status::timeout) {
            throw std::runtime_error("Database operation timed out");
        }
        
        return future.get();
    }
};

#endif