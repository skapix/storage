#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include <string>

namespace utilities
{

template <typename T, typename U>
auto makePathFile(const T & path, const U& file, const char c = '\\')
{
	return path.empty() || path.back() == c ?
		path + file :
		path + T(1, c) + file;
}

template <typename T>
T makePathFile(const T & path, const char * file, const char c = '\\')
{
	return file ?
		makePathFile<T, const char *>(path, file, c) :
		path;
}

std::string makePathFile(const char * path, const char * file, const char c = '\\');



	// make_unique with no except using new
	/*template <typename T>
	typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
		std::unique_ptr<T> >::type make_unique_no_except(const size_t val)
	{
		T * retVal = new (std::nothrow) T[val];
		return std::unique_ptr<T>(retVal);
	}

	template <typename T, typename ... Params>
	typename std::enable_if<!std::is_array<T>::value,
		std::unique_ptr<T> >::type make_unique_no_except(Params &&... params)
	{
		T * retVal = new (std::nothrow) T(std::forward(params...));
		return std::unique_ptr<T>(retVal);
	}*/

}