#ifndef INCLUDED_EXCEPTIONS_HXX
#define INCLUDED_EXCEPTIONS_HXX

//#include "concat/Utilities/Logging.hxx"

#define BEGIN_IGNORE_EXCEPTIONS \
	try \
	{

#define END_IGNORE_EXCEPTIONS(what) \
	} \
	catch (...) \
	{ \
		post("Exception caught and ignored (%s).", what); \
	}

#endif