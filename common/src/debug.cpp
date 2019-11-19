void debug_printf(const char* fmt, ...)
{
#ifndef NDEBUG
	/*
		String buffer on stack should be large enough for any required debug output. If output is
		cut off then try increasing to the next power of 2 size of 8192.
	*/
	char buffer[4096];

	// Format the output string with variable arguments and place in stack string buffer
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	// Display in debugger output window
	OutputDebugStringA(buffer);
#endif
}