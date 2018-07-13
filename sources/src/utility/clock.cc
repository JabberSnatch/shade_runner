/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "utility/clock.h"

namespace utility {


Clock::Clock(StepCallback_t _step_callback) :
	begin_{ StdClock_t::now() },
	step_point_{ begin_ },
	step_callback_{ _step_callback }
{}

float
Clock::read() const
{
	return StdDurationToSeconds(StdClock_t::now() - begin_);
}

void
Clock::step()
{
	StdClock_t::time_point new_step_point = StdClock_t::now();
	step_callback_(StdDurationToSeconds(new_step_point - step_point_));
	step_point_ = new_step_point;
}


} // namespace utility
