#include "util.h"

#include "memory.h"
#include "string.h"

#include <algorithm>
#include <cstring>
#include <psapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

void util::console::alloc( ) {
	AllocConsole( );

	freopen_s( reinterpret_cast< _iobuf** >( __acrt_iob_func( 0 ) ), "conin$", "r", static_cast< _iobuf* >( __acrt_iob_func( 0 ) ) );
	freopen_s( reinterpret_cast< _iobuf** >( __acrt_iob_func( 1 ) ), "conout$", "w", static_cast< _iobuf* >( __acrt_iob_func( 1 ) ) );
	freopen_s( reinterpret_cast< _iobuf** >( __acrt_iob_func( 2 ) ), "conout$", "w", static_cast< _iobuf* >( __acrt_iob_func( 2 ) ) );
}

void util::console::free( ) {
	fclose( static_cast< _iobuf* >( __acrt_iob_func( 0 ) ) );
	fclose( static_cast< _iobuf* >( __acrt_iob_func( 1 ) ) );
	fclose( static_cast< _iobuf* >( __acrt_iob_func( 2 ) ) );

	FreeConsole( );
}

address_t util::pattern_scan( const char* module_name, const char* signature ) noexcept {
	const auto module_handle = GetModuleHandleA( module_name );

	if ( !module_handle )
		return nullptr;

	static auto pattern_to_byte = []( const char* pattern ) {
		auto bytes = std::vector< int >{ };
		auto start = const_cast< char* >( pattern );
		auto end   = const_cast< char* >( pattern ) + std::strlen( pattern );

		for ( auto current = start; current < end; ++current ) {
			if ( *current == '?' ) {
				++current;

				if ( *current == '?' )
					++current;

				bytes.push_back( -1 );
			} else {
				bytes.push_back( std::strtoul( current, &current, 16 ) );
			}
		}
		return bytes;
	};

	auto dos_header = reinterpret_cast< PIMAGE_DOS_HEADER >( module_handle );
	auto nt_headers = reinterpret_cast< PIMAGE_NT_HEADERS >( reinterpret_cast< std::uint8_t* >( module_handle ) + dos_header->e_lfanew );

	auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
	auto pattern_bytes = pattern_to_byte( signature );
	auto scan_bytes    = reinterpret_cast< std::uint8_t* >( module_handle );

	auto s = pattern_bytes.size( );
	auto d = pattern_bytes.data( );

	for ( auto i = 0ul; i < size_of_image - s; ++i ) {
		bool found = true;

		for ( auto j = 0ul; j < s; ++j ) {
			if ( scan_bytes[ i + j ] != d[ j ] && d[ j ] != -1 ) {
				found = false;
				break;
			}
		}
		if ( found )
			return &scan_bytes[ i ];
	}
}

std::string util::ssprintf( const char* fmt, ... ) {
	va_list ap1, ap2;
	va_start( ap1, fmt );
	va_copy( ap2, ap1 );
	size_t sz = vsnprintf( NULL, 0, fmt, ap1 ) + 1;
	va_end( ap1 );
	char* buf = ( char* ) malloc( sz );
	vsnprintf( buf, sz, fmt, ap2 );
	va_end( ap2 );
	std::string str( buf );
	free( buf );
	return str;
}
