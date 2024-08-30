#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <memoryapi.h>
// clang-format on
#else
#include <dlfcn.h>
#include <sys/mman.h>
#define MAX_PATH 4096
#endif

#ifndef _WIN32
#define _stricmp strcasecmp
#endif

#include "address.h"
#include "math.h"

namespace util {
	namespace console {
		void alloc( );
		void free( );

		template < typename... args_t >
		void log( const char* fmt, const args_t&... args ) {
			printf( fmt, args... );
		}
	}  // namespace console

	address_t pattern_scan( const char* module_name, const char* signature ) noexcept;

	template < std::size_t index, typename t, typename... args_t >
	__forceinline t call_virtual( void* name, args_t... args ) {
		using fn_t = t( __thiscall* )( void*, args_t... );

		auto fn = ( *reinterpret_cast< fn_t** >( name ) )[ index ];
		return fn( name, args... );
	}

	template < std::size_t index >
	__forceinline address_t get_virtual( void* name ) {
		return address_t( ( *static_cast< int** >( name ) )[ index ] );
	}

	std::string ssprintf( const char* fmt, ... );
}  // namespace util
