#pragma once
#include <memory>
#include <type_traits>
#include <utility>

namespace utilities
{

void * allocate(const size_t sz);
void * reallocate(void * ptr, const size_t sz);
// TODO: move deallocate to Storage header
void deallocate(void * ptr);

template <typename T>
void destroy(T * t)
{
	if (t)
	{
		t->~();
		deallocate(t);
	}
}

template <typename T>
void deallocate_typed(T * t)
{
	deallocate(t);
}


template <typename T = char[], 
	typename Pointer = typename std::add_pointer<
	typename std::remove_extent<T>::type
	>::type>
class MemManager : public std::unique_ptr<T, void(*)(Pointer)>
{
	using Type = typename std::remove_pointer<Pointer>::type;
	using Func = void(*)(Pointer);
public:
	static typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0 && std::is_pod<T>::value,
		MemManager<T> >::type create(const size_t sz)
	{
		void * ptr = allocate(sz * sizeof(Type));
		//T * retVal = new (ptr) T[sz]; // placement new;
		MemManager<T> retVal((Pointer)ptr, deallocate_typed<Type>);
		return retVal;
	}

	// realloc
	typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0 && std::is_pod<T>::value,
		void >::type realloc(const size_t sz)
	{
		Pointer ptr = release();
		ptr = (Pointer)reallocate(ptr, sz);
		this->reset(ptr);
	}

	//static typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0 && !std::is_pod<T>::value,
	//	MemManager<T> >::type create(const size_t sz)
	//{
	//	void * ptr = allocate(sz * sizeof(T));
	//	T * retVal = new (ptr) T[sz]; // placement new;
	//	return MemManager<T>(std::unique_ptr<T>(retVal, destroy));
	//}

	//template <typename ... Params>
	//typename std::enable_if<!std::is_array<T>::value,
	//	MemManager<T> >::type create(Params &&... params)
	//{
	//	void * ptr = allocate(sizeof(T));
	//	T * retVal = new (ptr) T(std::forward(params)...); // placement new;
	//	return MemManager<T>(std::unique_ptr<T>(retVal, destroy));
	//}

	


private:
	//MemManager() : std::unique_ptr<T, void(*)(T*)>(nullptr, dest) {}
	MemManager(Pointer p, Func f)
		: std::unique_ptr<T, Func>(p, f) {}
	//MemManager(std::unique_ptr<T, Func> && buf)
	//	: std::unique_ptr(std::move(buf)) {}

private:
	/*std::unique_ptr<T, void(*)(T*)> m_buf;*/
};


	//template <typename T>
	//typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
	//	std::unique_ptr<T, void(*)(T*)> >::type make_unique_alloc(const size_t sz)
	//{
	//	void * ptr = allocate(sz * sizeof(T));
	//	T * retVal = new (ptr) T[sz]; // placement new;
	//	return std::unique_ptr<T>(retVal, destroy);
	//}

	//template <typename T, typename ... Params>
	//typename std::enable_if<!std::is_array<T>::value,
	//	std::unique_ptr<T, void(*)(T*)> >::type make_unique_alloc(Params &&... params)
	//{
	//	void * ptr = allocate(sizeof(T));
	//	T * retVal = new (ptr) T(std::forward(params)...); // placement new;
	//	return std::unique_ptr<T, void(*)(T*)>(retVal, destroy);
	//}
}