#pragma once

#include <thread>
#include <atomic>
#include <utility>
#include <functional>

#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/thread_context.hpp>
#include <mbgl/platform/platform.hpp>

#include <pthread.h>

namespace mbgl {
namespace util {

// Manages a thread with Object.

// Upon creation of this object, it launches a thread, creates an object of type Object in that
// thread, and then calls .start(); on that object. When the Thread<> object is destructed, the
// Object's .stop() function is called, and the destructor waits for thread termination. The
// Thread<> constructor blocks until the thread and the Object are fully created, so after the
// object creation, it's safe to obtain the Object stored in this thread.

template <class Object>
class Thread {
public:
    template <class... Args>
    Thread(const ThreadContext&, Args&&... args);
    ~Thread();

    // Invoke object->fn(args...) in the runloop thread.
    template <typename Fn, class... Args>
    void invoke(Fn fn, Args&&... args) {
        loop->invoke(bind(fn), std::forward<Args>(args)...);
    }

    // Invoke object->fn(args...) in the runloop thread, then invoke callback(result) in the current thread.
    template <typename Fn, class Cb, class... Args>
    std::unique_ptr<AsyncRequest>
    invokeWithCallback(Fn fn, Cb&& callback, Args&&... args) {
        return loop->invokeWithCallback(bind(fn), callback, std::forward<Args>(args)...);
    }

private:
    Thread(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread& operator=(Thread&&) = delete;

    template <typename Fn>
    auto bind(Fn fn) {
        return [fn, this] (auto &&... args) {
            return (object->*fn)(std::forward<decltype(args)>(args)...);
        };
    }

    template <typename P, std::size_t... I>
    void run(P&& params, std::index_sequence<I...>);

    std::atomic<bool> running;
    std::atomic<bool> joinable;

    std::thread thread;

    Object* object = nullptr;
    RunLoop* loop = nullptr;
};

template <class Object>
template <class... Args>
Thread<Object>::Thread(const ThreadContext& context, Args&&... args) : running(true), joinable(true) {
    // Note: We're using std::tuple<> to store the arguments because GCC 4.9 has a bug
    // when expanding parameters packs captured in lambdas.
    std::tuple<Args...> params = std::forward_as_tuple(::std::forward<Args>(args)...);

    thread = std::thread([&] {
#if defined(__APPLE__)
        pthread_setname_np(context.name.c_str());
#elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
        pthread_setname_np(pthread_self(), context.name.c_str());
#endif
#endif
        if (context.priority == ThreadPriority::Low) {
            platform::makeThreadLowPriority();
        }

        run(std::move(params), std::index_sequence_for<Args...>{});
    });

    while (running) {}
}

template <class Object>
template <typename P, std::size_t... I>
void Thread<Object>::run(P&& params, std::index_sequence<I...>) {
    RunLoop loop_(RunLoop::Type::New);
    loop = &loop_;

    Object object_(std::get<I>(std::forward<P>(params))...);
    object = &object_;

    running = false;
    loop_.run();

    loop = nullptr;
    object = nullptr;

    while (joinable) {}
}

template <class Object>
Thread<Object>::~Thread() {
    loop->stop();
    joinable = false;
    thread.join();
}

} // namespace util
} // namespace mbgl
