#ifndef NET_H
#define NET_H

#include <string>
#include <vector>

namespace Net
{
	void SendDiscoverabilityProbe();

	std::vector<std::string> GatherDiscoverabilityProbes( unsigned timeoutMS, unsigned maxProbes );

	int Listen();

	int Connect( const std::string &serverHostname );

	int SendData( int sock, const void *buf, unsigned bufLen );

	int RecvData( int sock, unsigned count, void *buf );

	namespace detail
	{
		int BufBuilderHelper( unsigned char *&ptr, unsigned *bytesLeft, const void *data, unsigned dataSize );
		int BufBuilder(unsigned char *&ptr, unsigned *bytesLeft, const char *arg);

		template<typename T>
		static int BufBuilder( unsigned char *&ptr, unsigned *bytesLeft, T arg )
		{
			return BufBuilderHelper( ptr, bytesLeft, &arg, sizeof( T ) );
		}

		template<typename T, typename ...Args>
		static int BufBuilder( unsigned char *&ptr, unsigned *bytesLeft, T arg, Args... args )
		{
			int bufSize = BufBuilderHelper( ptr, bytesLeft, &arg, sizeof( T ) );
			
			if ( bufSize )
			{
				int addedSize = BufBuilder( ptr, bytesLeft, std::forward<Args>( args )... );
				if ( addedSize )
					bufSize += addedSize;
				else
					bufSize = 0;
			}

			return bufSize;
		}

		template<typename ...Args>
		static int BufBuilder( unsigned char *&ptr, unsigned *bytesLeft, const char *arg, Args... args )
		{
			int bufSize = BufBuilder( ptr, bytesLeft, arg );

			if ( bufSize )
			{
				int addedSize = BufBuilder( ptr, bytesLeft, std::forward<Args>( args )... );
				if ( addedSize )
					bufSize += addedSize;
				else
					bufSize = 0;
			}

			return bufSize;
		}
	}

	template<typename ...Args>
	static int Send( int sock, Args... args )
	{
		unsigned char buf[4096];
		unsigned char *ptr = buf;
		unsigned bytesLeft = sizeof( buf );

		int bufSize = detail::BufBuilder( ptr, &bytesLeft, std::forward<Args>( args )... );

		if ( bufSize >= sizeof( buf ) )
			return -1;
		else
			return SendData( sock, buf, bufSize );
	}

	template<typename T>
	static T Recv(int sock)
	{
		unsigned char buf[sizeof( T )];
		RecvData( sock, sizeof( T ), buf );

		return *reinterpret_cast< T* >( buf );
	}

	template<typename T>
	static int Recv( int sock, T *var )
	{
		return RecvData( sock, sizeof( T ), var );
	}

	int Recv(int sock, std::string *var);
}

#endif // #ifndef NET_H
