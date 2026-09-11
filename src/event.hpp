#pragma once

#include <functional>
#include <vector>
#include <utility>

template <typename... Args>
struct Event {
    using Listener = std::function<void(Args...)>;

    std::vector<Listener> listeners;

    void addListener(Listener func) {
        listeners.push_back(std::move(func));
    }

    void removeListener(uint i) {
        listeners.erase(listeners.begin() + i);
    }

    void broadcast(const Args&... args) {
        for (const Listener& f : listeners) {
            if (f) {
                f(args...);
            }
        }
    }
};