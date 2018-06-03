/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_FILE_HPP__
#define __YS_FILE_HPP__


#include <ctime>
#include <string>


namespace utility {

class File
{
public:
	File() = default;
	File(std::string const &_path);
	bool Exists() const;
	std::string ReadAll();
	bool HasChanged() const;
private:
	std::string path_ = "";
	std::time_t read_time_ = static_cast<std::time_t>(0);
};

} // namespace utility


#endif __YS_FILE_HPP__
