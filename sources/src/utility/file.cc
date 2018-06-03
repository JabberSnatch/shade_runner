/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "utility/file.h"

#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>

namespace utility {

namespace boostfs = ::boost::filesystem;

File::File(std::string const &_path) :
	path_{ boostfs::absolute(boostfs::path{ _path }).generic_string() }
{
	assert(Exists());
}

bool File::Exists() const
{
	boostfs::path const fs_path{ path_ };
	return boostfs::is_regular_file(fs_path);
}

std::string
File::ReadAll()
{
	assert(Exists());
	std::stringstream string_stream{};
	std::fstream file_stream{ path_ };
	file_stream >> string_stream.rdbuf();
	read_time_ = std::time(nullptr);
	return string_stream.str();
}

bool
File::HasChanged() const
{
	assert(Exists());
	boostfs::path const fs_path{ path_ };
	return read_time_ < boostfs::last_write_time(fs_path);
}


} // namespace utility
