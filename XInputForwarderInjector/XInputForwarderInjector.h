#ifndef XINPUT_FORWARDER_INJECTOR_H
#define XINPUT_FORWARDER_INJECTOR_H

#ifdef BUILDING_DLL
# define DLL_EXPORT __declspec(dllexport)
#else
# define DLL_EXPORT __declspec(dllimport)
#endif

extern "C"
{
	struct XInputForwarderInjector;


	DLL_EXPORT extern XInputForwarderInjector* __cdecl CreateXInputForwarderInjector( int procId );


	DLL_EXPORT extern void __cdecl DestroyXInputForwarderInjector( XInputForwarderInjector *injector );


	DLL_EXPORT extern bool __cdecl InjectDLLAndCallFunction( XInputForwarderInjector *injector, const char *dllDir, const char *dllName, const char *functionName );
}

#undef DLL_EXPORT

#endif
