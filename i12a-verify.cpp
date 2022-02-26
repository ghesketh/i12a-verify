//
// i12a-verify.cpp
//
#include <GiHe.h>
#include <GiHe-nng.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <xxhash.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "base64.h"


int Encode( std::string & chars, const char * filename, XXH64_state_t * state = nullptr )
{
	int iResult = 0 ;

	do
	{
		std::ifstream ifstream( filename, std::ios::binary ) ;

		auto rdbuf = ifstream.rdbuf() ;

		std::vector< char > bytes( rdbuf->pubseekoff( 0, ifstream.end, ifstream.in ) ) ;

		rdbuf->pubseekpos( 0, ifstream.in ) ;

		rdbuf->sgetn( bytes.data(), bytes.size() ) ;

		ifstream.close() ;

		chars.resize( 1 + ( ( bytes.size() + 2 ) / 3 * 4 ) ) ;

		iResult = Base64encode( ( char * ) chars.data(), bytes.data(), int( bytes.size() ) ) ;

		if( nullptr != state )
		{
			iResult = XXH64_update( state, bytes.data(), bytes.size() ) ;
			//
			if( XXH_ERROR == iResult )
			{
				iResult = __LINE__ ;
				break ;
			}

		}

	}
	while( false ) ;

	return iResult ;

} // Encode()


int SendJSON( std::string & json, const char * uri )
{
	int iResult = 0 ;

	nng_http_res * http_res = nullptr ;
	nng_http_req * http_req = nullptr ;
	nng_http_conn * http_conn = nullptr ;
	nng_aio * aio = nullptr ;
	nng_http_client * http_client = nullptr ;
	nng_url * url = nullptr ;

	do
	{
		iResult = nng_url_parse( &( url ), uri ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_http_client_alloc( &( http_client ), url ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_aio_alloc( &( aio ), nullptr, nullptr ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		nng_http_client_connect( http_client, aio ) ;

		nng_aio_wait( aio ) ;

		iResult = nng_aio_result( aio ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		http_conn = ( nng_http_conn * ) nng_aio_get_output( aio, 0 ) ;

		iResult = nng_http_req_alloc( &( http_req ), url ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_http_req_set_method( http_req, "POST" ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_http_req_set_data( http_req, json.c_str(), json.size() ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		nng_http_conn_write_req( http_conn, http_req, aio ) ;

		nng_aio_wait( aio ) ;

		iResult = nng_aio_result( aio ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_http_res_alloc( &( http_res ) ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		nng_http_conn_read_res( http_conn, http_res, aio ) ;

		nng_aio_wait( aio ) ;

		iResult = nng_aio_result( aio ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = nng_http_res_get_status( http_res ) ;
		//
		if( NNG_HTTP_STATUS_OK != iResult )
		{
			iResult = __LINE__ ;

			fmt::print( stderr, "{} {}" ENDL, nng_http_res_get_status( http_res ), nng_http_res_get_reason( http_res ) ) ;

			break ;
		}

		auto header = nng_http_res_get_header( http_res, "Content-Length" ) ;

		if( nullptr == header )
		{
			iResult = __LINE__ ;

			std::cerr << "Content-Length:" << ENDL ;

			break ;
		}

		auto content_length = strtoul( header, nullptr, 0 ) ;

		std::string content ;
		//
		content.resize( content_length ) ;

		nng_iov iov ;
		//
		iov.iov_buf = content.data() ;
		iov.iov_len = content_length ;
		//
		iResult = nng_aio_set_iov( aio, 1, &( iov ) ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		nng_http_conn_read_all( http_conn, aio ) ;

		nng_aio_wait( aio ) ;

		iResult = nng_aio_result( aio ) ;
		//
		if( 0 != iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		std::cout << content << ENDL ;

	}
	while( false ) ;

	if( nullptr != http_res )
	{
		nng_http_res_free( http_res ) ;
	}

	if( nullptr != http_req )
	{
		nng_http_req_free( http_req ) ;
	}

	if( nullptr != http_conn )
	{
		nng_http_conn_close( http_conn ) ;
	}

	if( nullptr != aio )
	{
		nng_aio_free( aio ) ;
	}

	if( nullptr != http_client )
	{
		nng_http_client_free( http_client ) ;
	}

	if( nullptr != url )
	{
		nng_url_free( url ) ;
	}

	return iResult ;

} // SendJSON


int ReadJSON( std::string & json, const char * filename )
{
	int iResult = 0 ;

	do
	{
		std::stringstream buffer;

		std::ifstream ifstream( filename ) ;

		buffer << ifstream.rdbuf() ;

		ifstream.close() ;

		json = buffer.str() ;

	}
	while( false ) ;

	return iResult ;

} // ReadJSON()


int CreateJSON( std::string & json, std::vector< std::string > & files )
{
	int iResult = 0 ;

	XXH64_state_t * const state = XXH64_createState() ;

	do
	{
		if( nullptr == state )
		{
			iResult = __LINE__ ;
			break ;
		}

		iResult = XXH64_reset( state, nng_random() ) ;
		//
		if( XXH_ERROR == iResult )
		{
			iResult = __LINE__ ;
			break ;
		}

		std::vector< std::string > encoded ;

		for( auto & file : files )
		{
			std::string chars ;
			//
			iResult = Encode( chars, file.c_str(), state ) ;

			encoded.push_back( chars.c_str() ) ;

		} // for ... file

		if( 2 != encoded.size() )
		{
			iResult = __LINE__ ;
			break ;
		}

		json = fmt::format( "{{ \"idTransaccion\" : \"{:016x}\", \"fotoA\" : {{ \"tipo\" : 1, \"data\" : \"{}\" }}, \"fotoB\" : {{ \"tipo\" : 2, \"data\" : \"{}\" }} }}" ENDL, XXH64_digest( state ), encoded[ 0 ], encoded[ 1 ] ) ;

	}
	while( false ) ;

	if( nullptr != state )
	{
		XXH64_freeState( state ) ;
	}

	return iResult ;

} // CreateJSON()


int main( int iArgs, char * pArgs[] )
{
	int iResult = 0 ;

	do
	{
		std::string uri ;
		std::vector< std::string > files ;

		for( int iArg = 1 ; iArg < iArgs ; ++( iArg ) )
		{
			auto s = strlen( pArgs[ iArg ] ) ;
			//
			if( s < 2 )
			{
				continue ;
			}

			auto c = pArgs[ iArg ][ 0 ] ;

			if( '-' == c || '/' == c )
			{
				switch( pArgs[ iArg ][ 1 ] )
				{
					// URI
					//
					case 'U' :
					case 'u':
						if( iArgs > ++( iArg ) )
						{
							uri.assign( pArgs[ iArg ] ) ;
						}
						break ;

					default:
						break ;

				}

				continue ;
			}

			if( false == std::filesystem::is_regular_file( pArgs[ iArg ] ) )
			{
				continue ;
			}

			files.emplace_back( pArgs[ iArg ] ) ;

		} // for ... iArg

		switch( files.size() )
		{
			case 0 :
			{
				break ;
			}

			case 1 :
			{
				if( 0 != uri.size() )
				{
					// interpret the filename as a JSON file, post to server
					//
					std::string json ;

					iResult = ReadJSON( json, files[ 0 ].c_str() ) ;

					iResult = SendJSON( json, uri.c_str() ) ;
				}
				else
				{
					// interpret the filename as an image, output base64
					//
					std::string chars ;
					//
					iResult = Encode( chars, files[ 0 ].c_str(), nullptr ) ;

					std::cout << chars << ENDL ;
				}

				break ;
			}

			case 2 :
			{
				std::string json ;

				iResult = CreateJSON( json, files ) ;

				if( 0 != uri.size() )
				{
					iResult = SendJSON( json, uri.c_str() ) ;
				}
				else
				{
					std::cout << json << ENDL ;
				}

				break ;
			}

			default :
			{
				std::vector< std::string > tuples ;

				for( auto & file : files )
				{
					std::string chars ;
					//
					iResult = Encode( chars, file.c_str(), nullptr ) ;

					tuples.emplace_back( fmt::format( "\"{}\" : \"{}\"", file, chars ) ) ;
				}

				fmt::print( "{{ {} }}", fmt::join( tuples, "," ) ) ;

				break ;
			}

		} // switch ... files.size()

	}
	while( false ) ;

	return iResult ;

} // main()
