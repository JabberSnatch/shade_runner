/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_SHADERUNNER_HPP__
#define __YS_SHADERUNNER_HPP__


namespace sr {

class RenderContext
{
public:
	RenderContext();
	~RenderContext();

	bool RenderFrame();
	void WatchFKernelFile(char const *_path);
	void SetResolution(int _width, int _height);
private:
	struct Impl_;
	Impl_* impl_;
};

} // namespace sr

#endif __YS_SHADERUNNER_HPP__
