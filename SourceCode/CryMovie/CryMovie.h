
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CRYMOVIE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CRYMOVIE_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
	#ifdef CRYMOVIE_EXPORTS
		#define CRYMOVIE_API __declspec(dllexport)
	#else
		#define CRYMOVIE_API __declspec(dllimport)
	#endif
#else //WIN32
	#define CRYMOVIE_API
#endif //WIN32

struct ISystem;
struct IMovieSystem;

extern "C"
{

CRYMOVIE_API IMovieSystem *CreateMovieSystem(ISystem *pSystem);
CRYMOVIE_API void DeleteMovieSystem(IMovieSystem *pMM);

}