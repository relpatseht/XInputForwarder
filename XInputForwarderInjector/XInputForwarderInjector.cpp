#include "XInputForwarderInjector.h"
#include "RemoteCode.h"
#include "RemoteFunction.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct XInputForwarderInjector
{
	RemoteCode remoteCode;

	XInputForwarderInjector( int procId ) : remoteCode( procId )
	{
		remoteCode.Initialize();
	}

	~XInputForwarderInjector()
	{
	}
};


__declspec( dllexport ) XInputForwarderInjector* __cdecl CreateXInputForwarderInjector( int procId )
{
	return new XInputForwarderInjector( procId );
}


__declspec( dllexport ) void __cdecl DestroyXInputForwarderInjector( XInputForwarderInjector *injector )
{
	delete injector;
}


__declspec( dllexport ) bool __cdecl InjectDLLAndCallFunction( XInputForwarderInjector *injector, const char *dllDir, const char *dllName, const char *functionName )
{
	RemoteFunction dllDirectoryHandle = injector->remoteCode.GetFunction( "Kernel32.dll", "SetDllDirectoryA" );
	if ( !dllDirectoryHandle.IsValid() )
		return false;

	dllDirectoryHandle.SetCC( RemoteFunction::CC_STDCALL );
	dllDirectoryHandle.Call<unsigned>( dllDir );

	RemoteFunction functionHandle = injector->remoteCode.GetFunction( dllName, functionName );
	if ( !functionHandle.IsValid() )
		return false;

	unsigned functionSuccessful = functionHandle.Call<unsigned>();
	if ( !functionSuccessful )
		return false;

	return true;
}
