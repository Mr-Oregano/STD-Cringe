#ifndef CRINGE_PIPE_H
#define CRINGE_PIPE_H

#include <string>
#include <regex>
#include <cstdio>
#include <cctype>

class PipeGuard {
    FILE* pipe_;
public:
    explicit PipeGuard(FILE* pipe) : pipe_(pipe) {}
    ~PipeGuard() { if(pipe_) pclose(pipe_); }
    FILE* get() { return pipe_; }
    PipeGuard(const PipeGuard&) = delete;
    PipeGuard& operator=(const PipeGuard&) = delete;
};

bool is_safe_command(const std::string& cmd);
std::string sanitize_command(const std::string& cmd);

#endif