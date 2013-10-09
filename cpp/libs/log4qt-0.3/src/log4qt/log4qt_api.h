/*****************************************************************************
 * Copyright (C) 2011, BMW Car IT GmbH 
 *****************************************************************************/

//------------------------------------------------------------------------------
// Header that generates macros for a DLL based API when using Visual C++
//------------------------------------------------------------------------------

// On Windows you need to explicitly specify which classes/functions are 
// publically visible when creating a shared library.
// __declspec(dllexport) is a MSVC language extension to declare publically 
// visible classes/functions in the source code.
// Windows also requires that classes/functions appearing in an external shared 
// library be declared with the "__declspec(dllimport)" storage class.
// The __declspec is hidden inside a preprocessor macro for cross-platform 
// portability and to switch between dllexport and dllimport depending on
// whether the library is being built or being used.
#ifndef LOG4QT_API
	#ifdef _MSC_VER
		#ifdef BUILDING_LOG4QT
			#define LOG4QT_API __declspec(dllexport)
		#else
			#define LOG4QT_API __declspec(dllimport)
		#endif
	#else
		#define LOG4QT_API
	#endif
#endif
