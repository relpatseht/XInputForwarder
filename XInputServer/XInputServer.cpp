#include <iostream>
#include "Net.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <WinSock2.h>

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

int main()
{
	WSAData wsaData;
	bool ret;

	ret = WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == 0;

	std::vector<std::string> hosts = Net::GatherDiscoverabilityProbes( 2000, 20 );
	while(hosts.size() == 0)
		hosts = Net::GatherDiscoverabilityProbes( 2000, 20 );

	int sock = Net::Connect( hosts[0] );

	if ( sock < 0 )
	{
		ret = false;
	}
	else
	{
		XInputEnable( TRUE );

		for ( ;;)
		{
			switch ( Net::Recv<unsigned>( sock ) )
			{
				case ENABLE:
					XInputEnable( Net::Recv<BOOL>( sock ) );
				break;
				case GET_AUDIO_DEVICE_IDS:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					UINT inRenderCount = Net::Recv<UINT>( sock );
					UINT inCaptureCount = Net::Recv<UINT>( sock );
					WCHAR renderDevId[1024];
					WCHAR captureDevId[1024];

					DWORD ret = XInputGetAudioDeviceIds( userIndex, renderDevId, &inRenderCount, captureDevId, &inCaptureCount );
					Net::Send( sock, 0, 0, ret );
				}
				break;
				case GET_BATTERY_INFORMATION:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					BYTE devType = Net::Recv<BYTE>( sock );
					XINPUT_BATTERY_INFORMATION batInfo;

					DWORD ret = XInputGetBatteryInformation( userIndex, devType, &batInfo );
					Net::Send( sock, batInfo, ret );
				}
				break;
				case GET_CAPABILITIES:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					DWORD flags = Net::Recv<DWORD>( sock );
					XINPUT_CAPABILITIES capabilities;

					DWORD ret = XInputGetCapabilities( userIndex, flags, &capabilities );
					Net::Send( sock, capabilities, ret );
				}
				break;
				/*case GET_DSOUND_AUDIO_DEVICE_GUIDS:
				{
					DWORD userIndex = Recv<DWORD>( sock );
					GUID render;
					GUID capture;

					DWORD ret = XInputGetDSoundAudioDeviceGuids()
				}
				break;*/
				case GET_KEYSTROKE:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					DWORD reserved = Net::Recv<DWORD>( sock );
					XINPUT_KEYSTROKE keystroke;

					DWORD ret = XInputGetKeystroke( userIndex, reserved, &keystroke );
					Net::Send( sock, keystroke, ret );
				}
				break;
				case GET_STATE:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					XINPUT_STATE state;

					DWORD ret = XInputGetState( userIndex, &state );
					Net::Send( sock, state, ret );
				}
				break;
				case SET_STATE:
				{
					DWORD userIndex = Net::Recv<DWORD>( sock );
					XINPUT_VIBRATION vibe = Net::Recv<XINPUT_VIBRATION>( sock );

					DWORD ret = XInputSetState( userIndex, &vibe );
					Net::Send( sock, ret );
				}
				break;
			}
		}
	}

	return ret ? 0 : -1;
}
