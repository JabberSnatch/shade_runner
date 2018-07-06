/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_TIMER_HPP__
#define __YS_TIMER_HPP__

#include <chrono>
#include <functional>


namespace utility {


struct NoOpCallback{ void operator()(float) const {} };


class Timer
{
public:
	using ExitCallback_t = std::function<void(float)>;
public:
	using StdClock_t = std::chrono::high_resolution_clock;
	template <typename Rep, typename Period>
	constexpr float StdDurationToSeconds(std::chrono::duration<Rep, Period> const &_d)
	{ return std::chrono::duration_cast<std::chrono::duration<float>>(_d).count(); }
public:
	Timer(ExitCallback_t _exit_callback = NoOpCallback{});
	~Timer();
private:
	StdClock_t::time_point begin_;
	ExitCallback_t exit_callback_;
};


} // namespace utility


#endif // __YS_TIMER_HPP__
