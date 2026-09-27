#pragma once
#include <vector>
#include <functional>

template<typename... Args>
class Event
{
public:
    using Listener = std::function<void(Args...)>;

    // Subscribe a callback function or lambda to this event
    void addListener(Listener listener)
    {
        listeners.push_back(listener);
    }

    // Trigger the event and notify all subscribers
    void invoke(Args... args) const
    {
        for (const auto& listener : listeners)
        {
            if (listener)
            {
                listener(args...);
            }
        }
    }

private:
    std::vector<Listener> listeners;
};