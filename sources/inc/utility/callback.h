/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_CALLBACK_HPP__
#define __YS_CALLBACK_HPP__

#include <functional>
#include <vector>

namespace utility {

template <typename ... Args>
class Callback
{
public:
    void operator()(Args ... args) {
        for (auto &&cb : listeners_) cb(args...);
    }
    std::vector<std::function<void(Args...)>> listeners_;
};

} // namespace utility

#endif // __YS_CALLBACK_HPP__
