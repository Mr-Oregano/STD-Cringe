#ifndef CRINGE_ASYNC_H
#define CRINGE_ASYNC_H

#include <coroutine>
#include <exception>
#include <future>
#include <memory>
#include <string>

template<typename T>
class Task {
public:
    struct promise_type {
        Task<T> get_return_object() { 
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; 
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_value(T value) { result = std::move(value); }
        void unhandled_exception() { 
            exception_ptr = std::current_exception(); 
        }
        T result;
        std::exception_ptr exception_ptr;
    };

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const { }
    T await_resume() const { 
        if (handle.promise().exception_ptr) {
            std::rethrow_exception(handle.promise().exception_ptr);
        }
        return std::move(handle.promise().result);
    }

    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { 
        if (handle) handle.destroy(); 
    }

private:
    std::coroutine_handle<promise_type> handle;
};

#endif // CRINGE_ASYNC_H