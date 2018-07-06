/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "utility/timer.h"

namespace utility {


Timer::Timer(ExitCallback_t _exit_callback) :
	begin_{ StdClock_t::now() },
	exit_callback_{ _exit_callback }
{}

Timer::~Timer()
{
	StdClock_t::duration const delta = StdClock_t::now() - begin_;
	exit_callback_(StdDurationToSeconds(delta));
}


} // namespace utility
