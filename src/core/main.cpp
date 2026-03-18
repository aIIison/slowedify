#include "hooks.h"
#include "util/util.h"

#include <chrono>
#include <thread>

bool b_release = false;

util::console::cmd_t release_cmd( "release", "release - release the dll from the program.", []( util::console::cmd_t::args_t args ) -> bool {
#ifdef _DEBUG
	util::console::free( );
#endif

	b_release = true;
	return true;
} );

util::console::cmd_t speed( "speed", "speed <speed> - set playback speed.", []( util::console::cmd_t::args_t args ) -> bool {
	if ( args.size( ) > 1 ) {
		auto speed = std::stof( args[ 1 ] );
		if ( speed > 1.f ) {
			speed = 1.f;
			util::console::log_error( "[!] playback speed above 1.0 is not supported!\n" );
		}

		hooks::set_speed( speed );

		return true;
	}
	return false;
} );

unsigned long WINAPI initialize( void* instance ) {
#ifdef _DEBUG
	util::console::alloc( );
#endif

	util::console::log( "[+] allocated console.\n" );

	hooks::initialize( );

	while ( !b_release )
		std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

	FreeLibraryAndExitThread( static_cast< HMODULE >( instance ), 0 );
}

unsigned long WINAPI release( ) {
	hooks::uninitialize( );

#ifdef _DEBUG
	util::console::free( );
#endif

	return TRUE;
}

std::int32_t WINAPI DllMain( const HMODULE instance [[maybe_unused]], const unsigned long reason, const void* reserved [[maybe_unused]] ) {
	DisableThreadLibraryCalls( instance );

	switch ( reason ) {
	case DLL_PROCESS_ATTACH: {
		if ( auto handle = CreateThread( nullptr, NULL, initialize, instance, NULL, nullptr ) )
			CloseHandle( handle );

		break;
	}

	case DLL_PROCESS_DETACH: {
		release( );
		break;
	}
	}

	return true;
}
