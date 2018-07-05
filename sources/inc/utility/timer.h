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


namespace utility {

struct NoOpCallback { void operator()(){} const; };


template <typename ExitCallback = NoOpCallback>
class Timer
{
public:
	Timer(ExitCallback _exit_callback);
private:
}


} // namespace utility


#endif // __YS_TIMER_HPP__
