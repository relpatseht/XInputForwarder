#include "Net.h"
#include "FuncHookerFactory.h"
#include <atomic>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <Xinput.h>
#include <WS2tcpip.h>


namespace
{
	std::atomic<int> g_sock = -1;
}

namespace XInputHook
{
	void *XInputEnablePtr = nullptr;
	void *XInputGetAudioDeviceIdsPtr = nullptr;
	void *XInputGetBatteryInformationPtr = nullptr;
	void *XInputGetCapabilitiesPtr = nullptr;
	void *XInputGetDSoundAudioDeviceGuidsPtr = nullptr;
	void *XInputGetKeystrokePtr = nullptr;
	void *XInputGetStatePtr = nullptr;
	void *XInputSetStatePtr = nullptr;

	enum
	{
		ENABLE,
		GET_AUDIO_DEVICE_IDS,
		GET_BATTERY_INFORMATION,
		GET_CAPABILITIES,
		GET_DSOUND_AUDIO_DEVICE_GUIDS,
		GET_KEYSTROKE,
		GET_STATE,
		SET_STATE
	};

	void WINAPI XInputEnable
	(
		_In_ BOOL enable     // [in] Indicates whether xinput is enabled or disabled. 
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), ENABLE, enable );
		}
		else
		{
			auto XInputEnableHooker = FuncHookerCast( XInputEnablePtr, XInputEnable );
			return XInputEnableHooker->CallInjectee( enable );
		}
	}

	DWORD WINAPI XInputGetAudioDeviceIds
	(
		_In_  DWORD                             dwUserIndex,        // Index of the gamer associated with the device
		_Out_writes_opt_( *pRenderCount ) LPWSTR  pRenderDeviceId,    // Windows Core Audio device ID string for render (speakers)
		_Inout_opt_ UINT*                       pRenderCount,       // Size of render device ID string buffer (in wide-chars)
		_Out_writes_opt_( *pCaptureCount ) LPWSTR pCaptureDeviceId,   // Windows Core Audio device ID string for capture (microphone)
		_Inout_opt_ UINT*                       pCaptureCount       // Size of capture device ID string buffer (in wide-chars)
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_AUDIO_DEVICE_IDS, dwUserIndex, *pRenderCount, *pCaptureCount );
			*pRenderCount = Net::Recv<UINT>( g_sock.load());
			Net::RecvData( g_sock.load(), *pRenderCount * sizeof( WCHAR ), ( unsigned char* ) pRenderDeviceId );

			*pCaptureCount = Net::Recv<UINT>( g_sock.load());
			Net::RecvData( g_sock.load(), *pCaptureCount * sizeof( WCHAR ), ( unsigned char* ) pCaptureDeviceId );

			return Net::Recv<DWORD>( g_sock.load());
		}
		else
		{
			auto XInputGetAudioDeviceIdsHooker = FuncHookerCast( XInputGetAudioDeviceIdsPtr, XInputGetAudioDeviceIds );
			return XInputGetAudioDeviceIdsHooker->CallInjectee( dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount );
		}
	}

	DWORD WINAPI XInputGetBatteryInformation
	(
		_In_  DWORD                       dwUserIndex,        // Index of the gamer associated with the device
		_In_  BYTE                        devType,            // Which device on this user index
		_Out_ XINPUT_BATTERY_INFORMATION* pBatteryInformation // Contains the level and types of batteries
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_BATTERY_INFORMATION, dwUserIndex, devType );

			Net::Recv( g_sock.load(), pBatteryInformation );

			return Net::Recv<DWORD>( g_sock.load() );
		}
		else
		{
			auto XInputGetBatteryInformationHooker = FuncHookerCast( XInputGetBatteryInformationPtr, XInputGetBatteryInformation );
			return XInputGetBatteryInformationHooker->CallInjectee( dwUserIndex, devType, pBatteryInformation );
		}
	}

	DWORD WINAPI XInputGetCapabilities
	(
		_In_  DWORD                dwUserIndex,   // Index of the gamer associated with the device
		_In_  DWORD                dwFlags,       // Input flags that identify the device type
		_Out_ XINPUT_CAPABILITIES* pCapabilities  // Receives the capabilities
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_CAPABILITIES, dwUserIndex, dwFlags );

			Net::Recv( g_sock.load(), pCapabilities);

			return Net::Recv<DWORD>( g_sock.load() );
		}
		else
		{
			auto XInputGetCapabilitiesHooker = FuncHookerCast( XInputGetCapabilitiesPtr, XInputGetCapabilities );
			return XInputGetCapabilitiesHooker->CallInjectee( dwUserIndex, dwFlags, pCapabilities );
		}
	}

	DWORD WINAPI XInputGetDSoundAudioDeviceGuids
	(
		_In_  DWORD     dwUserIndex,          // Index of the gamer associated with the device
		_Out_ GUID*     pDSoundRenderGuid,    // DSound device ID for render (speakers)
		_Out_ GUID*     pDSoundCaptureGuid    // DSound device ID for capture (microphone)
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_DSOUND_AUDIO_DEVICE_GUIDS, dwUserIndex );

			Net::Recv( g_sock.load(), pDSoundRenderGuid);
			Net::Recv( g_sock.load(), pDSoundCaptureGuid);

			return Net::Recv<DWORD>( g_sock.load() );
		}
		else
		{
			auto XInputGetDSoundAudioDeviceGuidsHooker = FuncHookerCast( XInputGetDSoundAudioDeviceGuidsPtr, XInputGetDSoundAudioDeviceGuids );
			return XInputGetDSoundAudioDeviceGuidsHooker->CallInjectee( dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid );
		}
	}

	DWORD WINAPI XInputGetKeystroke
	(
		_In_       DWORD dwUserIndex,              // Index of the gamer associated with the device
		_Reserved_ DWORD dwReserved,               // Reserved for future use
		_Out_      PXINPUT_KEYSTROKE pKeystroke    // Pointer to an XINPUT_KEYSTROKE structure that receives an input event.
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_KEYSTROKE, dwUserIndex, dwReserved );

			Net::Recv( g_sock.load(), pKeystroke);

			return Net::Recv<DWORD>( g_sock.load());
		}
		else
		{
			auto XInputGetKeystrokeHooker = FuncHookerCast( XInputGetKeystrokePtr, XInputGetKeystroke );
			return XInputGetKeystrokeHooker->CallInjectee( dwUserIndex, dwReserved, pKeystroke );
		}
	}

	DWORD WINAPI XInputGetState
	(
		_In_  DWORD         dwUserIndex,  // Index of the gamer associated with the device
		_Out_ XINPUT_STATE* pState        // Receives the current state
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), GET_STATE, dwUserIndex );

			Net::Recv( g_sock.load(), pState);

			return Net::Recv<DWORD>( g_sock.load());
		}
		else
		{
			auto XInputGetStateHooker = FuncHookerCast( XInputGetStatePtr, XInputGetState );
			return XInputGetStateHooker->CallInjectee( dwUserIndex, pState );
		}
	}

	DWORD WINAPI XInputSetState
	(
		_In_ DWORD             dwUserIndex,  // Index of the gamer associated with the device
		_In_ XINPUT_VIBRATION* pVibration    // The vibration information to send to the controller
	)
	{
		if ( g_sock.load() >= 0 )
		{
			Net::Send( g_sock.load(), SET_STATE, dwUserIndex, *pVibration );

			return Net::Recv<DWORD>( g_sock.load());
		}
		else
		{
			auto XInputSetStateHooker = FuncHookerCast( XInputSetStatePtr, XInputSetState );
			return XInputSetStateHooker->CallInjectee( dwUserIndex, pVibration );
		}
	}
}

namespace Server
{
	static void DiscoverThread()
	{
		do
		{
			Net::SendDiscoverabilityProbe();
			Sleep( 500 );
		} while ( g_sock.load() < 0 );
	}

	static void ConnectThread( )
	{
		for ( ;;)
		{
			int clientSock = Net::Listen();
			if ( clientSock >= 0 )
			{
				g_sock.store( clientSock );
				break;
			}

			Sleep( 100 );
		}
	}

	static void MonitorThread()
	{
		for ( ;;)
		{
			int err;
			int len = sizeof( err );
			int ret = getsockopt( g_sock.load(), SOL_SOCKET, SO_ERROR, (char*)&err, &len );

			if ( ret < 0 || err != 0 )
			{
				std::thread discoverThread( DiscoverThread );
				std::thread connectThread( ConnectThread );
				discoverThread.join();
				connectThread.join();
			}
			else
			{
				Sleep( 1000 );
			}
		}
	}
}

extern "C"
{
	__declspec( dllexport ) extern unsigned InitXInputForward()
	{
		bool ret;
		WSAData wsaData;

		ret = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == 0;

	#define INSTALL_HOOK(FUNC_NAME)														        \
		auto FUNC_NAME ## _Hooker = CreateFuncHooker( #FUNC_NAME, XInputHook:: ## FUNC_NAME );	\
		XInputHook::FUNC_NAME ## Ptr = FUNC_NAME ## _Hooker;                                    \
		if ( !( FUNC_NAME ## _Hooker ) || !( FUNC_NAME ## _Hooker )->InstallHook() )	        \
			ret &= false;


		INSTALL_HOOK(XInputEnable)
		INSTALL_HOOK(XInputGetAudioDeviceIds)
		INSTALL_HOOK(XInputGetBatteryInformation)
		INSTALL_HOOK(XInputGetCapabilities)
		//INSTALL_HOOK(XInputGetDSoundAudioDeviceGuids)
		INSTALL_HOOK(XInputGetKeystroke)
		INSTALL_HOOK(XInputGetState)
		INSTALL_HOOK(XInputSetState)

		std::thread monitorThread( Server::MonitorThread);
		monitorThread.detach();

		return ret ? 1 : 0;
	}
}