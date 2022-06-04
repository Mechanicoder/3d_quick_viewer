// Mechanicoder
// 2022/06/04

#pragma once

#include <mutex>
#include <queue>

template <class T>
class BlockQueue
{
public:
    void Push(const T& t)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _q.push(t);
    }

    template <class Iterator>
    void Push(const Iterator& begin, const Iterator& end)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        auto it = begin;
        while (it != end)
        {
            _q.push(*it);
            ++it;
        }
    }

    bool NotEmptyThenPop(T& t)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_q.empty())
        {
            return false;
        }

        t = _q.front();
        _q.pop();
        return true;
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _q.empty();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        while (!_q.empty())
        {
            _q.pop();
        }
    }

private:
    std::mutex _mtx;
    std::queue<T> _q;
};