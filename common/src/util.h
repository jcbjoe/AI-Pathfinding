/*
	Casts a COM pointer to anothwer type and checks that the cast was successful.
*/
template<typename T>
T* com_cast(IUnknown* src)
{
	T* dst;
	check_hresult(src->QueryInterface(__uuidof(T), (void**)&dst));

	return dst;
}

/*
	Attempts to cast a COM pointer to another type and returns nullptr if the cast was unsuccessful.
*/
template<typename T>
T* try_com_cast(IUnknown* src)
{
	T* dst;
	src->QueryInterface(__uuidof(T), (void**)&dst);

	return dst;
}