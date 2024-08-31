#include "hooks.h"
#include "util/util.h"

#include <chrono>
#include <thread>

unsigned long WINAPI initialize( void* instance ) {
#ifdef _DEBUG
	util::console::alloc( );
#endif

	util::console::log( "[+] allocated console.\n" );

	hooks::initialize( );

	while ( !GetAsyncKeyState( VK_END ) )
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
