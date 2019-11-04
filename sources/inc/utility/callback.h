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
#include <stdexcept>
#include <vector>

namespace utility {

template <typename ... Args>
struct Callback
{
    void operator()(Args ... args) {
        for (auto &&cb : listeners_) cb(args...);
    }
    std::vector<std::function<void(Args...)>> listeners_;
};

template <typename Return>
struct Query
{
    Return operator()(){
        if (source_) return source_();
        throw std::logic_error("Query called without any source bound");
    }
    std::function<Return()> source_;
};

} // namespace utility

#endif // __YS_CALLBACK_HPP__
