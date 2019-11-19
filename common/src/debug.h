/*
	Helper macros used for an old trick to make other macros behave more like standard C/C++
	function calls.

	Explanation:
		#define assert(condition)

		if (!error)
			do_something();
		else
			assert(false);
		do_something_else(); // Will only get called if error occured because assert macro is empty
*/
#define macro_begin	do{
#define macro_end	}while(0)

// Disable standard assert macro so we can define our own version
#ifdef assert
	#undef assert
#endif

/*
	The C++ standard uses NDEBUG (not debug) to determine if assert calls should be removed. We can
	reuse it for our custom assert and other debug macros.
*/
#ifdef NDEBUG
	#define debug_break()		macro_begin macro_end
	#define assert(condition)	macro_begin macro_end
	#define check_hresult(call) macro_begin call; macro_end
#else
	/*
		Forces the debugger to break on the line where this macro is called. This is useful in
		situations where an error has occured and we want to print out custom debug information
		before breaking in the debugger,
	*/
	#define debug_break() __debugbreak()

	/*
		Custom assert macro which is a bit nicer to use than the standard Visual C++ version.

		The standard assert macro brings up a confusing dialog box and does not break at the actual
		line where the assertion failed. This version outputs the file, line number and condition to
		the debugger output window, and then breaks on the corrent source line.

		Remember that you can use F5/F10/F11 to continue while debugging.

		Asserts should only be used to catch programming errors, for example to check that the
		arguments passed to a function are valid, or that an OS function call succeeded. Never use
		asserts for validating data provided by the end user. To an artist an assert will just
		appear to be a crash, and for a gamer anything could happen as asserts are stripped out in
		release builds.
	*/
	#define assert(condition) \
		macro_begin \
		if (!(condition)) \
		{ \
			debug_printf("%s(%d): Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
			__debugbreak(); \
		} \
		macro_end

	/*
		Macro for checking the results of a Windows API function call that returns an HRESULT.

		In debug builds if an API call fails it will print the error code to the debugger output
		window and then break in the debugger. In release builds it will perform the API call
		without checking the result.
	*/ 
	#define check_hresult(call) \
		macro_begin \
		long result = call; \
		if (result != 0) \
		{ \
			debug_printf("%s(%d): HRESULT failure: 0x%08X\n", __FILE__, __LINE__, result); \
			__debugbreak(); \
		} \
		macro_end
#endif

/*
	Prints a formatted string to the debugger output window in debug builds. In release builds this
	function does nothing and will be stripped entirely from the executable.
*/
void debug_printf(const char* fmt, ...);