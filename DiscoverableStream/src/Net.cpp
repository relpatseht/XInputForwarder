#include "Net.h"
#include <chrono>
#include <algorithm>
#include <unordered_set>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace Net
{
	namespace
	{
		static const unsigned short DISC_PORT = 65432;
		static const char * const DISC_ADDR = "239.1.2.3";
		static const unsigned short SERV_PORT = 45325;
	}

	namespace detail
	{
		int BufBuilderHelper( unsigned char *&ptr, unsigned *bytesLeft, const void *data, unsigned dataSize )
		{
			if ( dataSize > *bytesLeft )
			{
				return 0;
			}
			else
			{
				memcpy( ptr, data, dataSize );
				ptr += dataSize;
				*bytesLeft -= dataSize;
			}

			return dataSize;
		}

		int BufBuilder(unsigned char *&ptr, unsigned *bytesLeft, const char *arg)
		{
			unsigned argLen = strlen(arg) + 1;
			int bufSize = BufBuilder(ptr, bytesLeft, argLen);
			if (bufSize)
			{
				int addedSize = BufBuilderHelper(ptr, bytesLeft, arg, argLen);
				if (addedSize)
					bufSize += addedSize;
				else
					bufSize = 0;
			}

			return bufSize;
		}

		static int DisableNagle( int sock )
		{
			int yes = 1;
			if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, ( const char* ) &yes, sizeof( yes ) ) < 0 )
			{
				closesocket( sock );
				sock = -1;
			}

			return sock;
		}
	}


	static int MakeDiscoverabilitySock()
	{
		int sock = socket( AF_INET, SOCK_DGRAM, 0 );

		if ( sock >= 0 )
		{
			bool ret = true;
			unsigned yes = 1;

			if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &yes, sizeof( yes ) ) < 0 )
			{
				ret = false;
			}
			else
			{
				struct sockaddr_in local;

				memset( &local, 0, sizeof( local ) );
				local.sin_family = AF_INET;
				local.sin_port = htons( DISC_PORT );
				local.sin_addr.s_addr = htonl( INADDR_ANY );

				if ( bind( sock, ( sockaddr* ) &local, sizeof( local ) ) < 0 )
				{
					ret = false;
				}
				else
				{
					struct ip_mreq multi;

					multi.imr_multiaddr.s_addr = inet_addr( DISC_ADDR );
					multi.imr_interface.s_addr = htonl( INADDR_ANY );

					if ( setsockopt( sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, ( const char* ) &multi, sizeof( multi ) ) < 0 )
					{
						ret = false;
					}
					else
					{
						if ( setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP, ( const char* ) &yes, sizeof( yes ) ) < 0 )
						{
							ret = false;
						}
					}
				}
			}

			if ( !ret )
			{
				closesocket( sock );
				sock = -1;
			}
		}

		return sock;
	}


	static void CloseDiscoverabilitySock( int discSock )
	{
		struct ip_mreq multi;

		multi.imr_multiaddr.s_addr = inet_addr( DISC_ADDR );
		multi.imr_interface.s_addr = htonl( INADDR_ANY );

		setsockopt( discSock, IPPROTO_IP, IP_DROP_MEMBERSHIP, ( const char* ) &multi, sizeof( multi ) );
		closesocket( discSock );
	}


	void SendDiscoverabilityProbe()
	{
		int sock = socket( AF_INET, SOCK_DGRAM, 0 );
		sockaddr_in multiAddr;
		char hostname[1024];

		memset( &multiAddr, 0, sizeof( multiAddr ) );
		multiAddr.sin_family = AF_INET;
		multiAddr.sin_port = htons( DISC_PORT );
		multiAddr.sin_addr.s_addr = inet_addr( DISC_ADDR );

		gethostname( hostname, sizeof( hostname ) );

		sendto( sock, hostname, strlen( hostname ) + 1, 0, ( sockaddr* ) &multiAddr, sizeof( multiAddr ) );

		closesocket( sock );
	}

	std::vector<std::string> GatherDiscoverabilityProbes( unsigned timeoutMS, unsigned maxProbes )
	{
		std::unordered_set<std::string> servers;
		int sock = MakeDiscoverabilitySock();

		while ( timeoutMS > 0 && servers.size() < maxProbes )
		{
			timeval timeLeft;
			timeLeft.tv_sec = timeoutMS / 1000;
			timeLeft.tv_usec = ( timeoutMS - ( timeLeft.tv_sec * 1000 ) ) * 1000;

			fd_set readSet;
			FD_ZERO( &readSet );
			FD_SET( sock, &readSet );

			auto startTime = std::chrono::high_resolution_clock::now();

			int ret = select( sock + 1, &readSet, nullptr, nullptr, &timeLeft );
			if ( ret > 0 )
			{
				auto endTime = std::chrono::high_resolution_clock::now();

				unsigned selectTimeMS = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
				timeoutMS -= std::min( timeoutMS, selectTimeMS );

				if ( FD_ISSET( sock, &readSet ) )
				{
					char servHost[256];
					sockaddr_in servAddr;
					int servAddrLen = sizeof( servAddr );

					recvfrom( sock, servHost, sizeof( servHost ), 0, ( sockaddr* ) &servAddr, &servAddrLen );

					servers.insert( servHost );
				}
			}
			else
			{
				break;
			}
		}

		CloseDiscoverabilitySock( sock );

		return std::vector<std::string>( servers.begin(), servers.end() );
	}

	int Listen()
	{
		struct addrinfo hints, *res;
		const struct addrinfo *localAddr;
		char portStr[16];
		char hostname[1024];

		gethostname( hostname, sizeof( hostname ) );

		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		getaddrinfo( hostname, _itoa( SERV_PORT, portStr, 10 ), &hints, &res );

		localAddr = nullptr;
		for ( const addrinfo *addr = res; addr; addr = addr->ai_next )
		{
			if ( addr->ai_family == AF_INET )
			{
				sockaddr_in *ip4Addr = reinterpret_cast<sockaddr_in*>( addr->ai_addr );

				if ( ip4Addr->sin_addr.s_addr != INADDR_ANY && ip4Addr->sin_addr.s_addr != INADDR_LOOPBACK )
				{
					localAddr = addr;
					break;
				}
			}
		}

		int lSock = socket( localAddr->ai_family, localAddr->ai_socktype, localAddr->ai_protocol );
		if ( lSock >= 0 )
		{
			bool ret = true;
			int yes = 1;

			if ( setsockopt( lSock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &yes, sizeof( yes ) ) < 0 )
			{
				ret = false;
			}
			else
			{
				if ( bind( lSock, localAddr->ai_addr, localAddr->ai_addrlen ) < 0 )
				{
					ret = false;
				}
				else
				{
					if ( listen( lSock, 8 ) < 0 )
					{
						ret = false;
					}
					else
					{
						int newSock = accept( lSock, nullptr, nullptr );
						if ( newSock < 0 )
						{
							char dbgStr[1024];
							sprintf( dbgStr, "failed %d %d\n", newSock, WSAGetLastError() );
							OutputDebugString( dbgStr );
							ret = false;
						}
						else
						{
							closesocket( lSock );
							lSock = detail::DisableNagle( newSock );
						}
					}
				}
			}

			if ( !ret )
			{
				closesocket( lSock );
				lSock = -1;
			}
		}

		freeaddrinfo( res );

		return lSock;
	}

	int Connect( const std::string &serverHostname )
	{
		struct addrinfo hints, *res;
		char portStr[16];
		int servSock;

		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		getaddrinfo( serverHostname.c_str(), _itoa( SERV_PORT, portStr, 10 ), &hints, &res );

		servSock = -1;
		for ( const addrinfo *addr = res; addr; addr = addr->ai_next )
		{
			int sock = socket( addr->ai_family, addr->ai_socktype, addr->ai_protocol );
			if ( sock >= 0 )
			{
				if ( connect( sock, addr->ai_addr, addr->ai_addrlen ) < 0 )
				{
					closesocket( sock );
				}
				else
				{
					servSock = sock;
					break;
				}
			}
		}

		freeaddrinfo( res );

		return detail::DisableNagle( servSock );
	}

	int SendData( int sock, const void *buf, unsigned bufLen )
	{
		const char *bufPtr = reinterpret_cast< const char* >( buf );

		unsigned totalSent = 0;
		do
		{
			int sent = send( sock, bufPtr, bufLen - totalSent, 0 );
			if ( sent <= 0 )
			{
				return sent;
			}

			totalSent += sent;
			bufPtr += sent;
		} while ( totalSent < bufLen );

		return totalSent;
	}

	int RecvData( int sock, unsigned count, void *buf )
	{
		char *bufPtr = reinterpret_cast< char* >( buf );

		unsigned totalRecvd = 0;
		do
		{
			int recvd = recv( sock, bufPtr, count - totalRecvd, 0 );
			if ( recvd <= 0 )
			{
				return recvd;
			}

			totalRecvd += recvd;
			bufPtr += recvd;
		} while ( totalRecvd < count );

		return totalRecvd;
	}


	int Recv(int sock, std::string *var)
	{
		unsigned strLen;

		int recvd = Recv(sock, &strLen);
		if (recvd > 0)
		{
			char *str = (char*)alloca(strLen);

			recvd = RecvData(sock, strLen, str);
			if (recvd > 0)
			{
				recvd += sizeof(unsigned);
				*var = str;
			}
		}

		return recvd;
	}
}
