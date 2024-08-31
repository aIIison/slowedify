#include "hooks.h"

#include "util/util.h"

subhook_t create_player_hook;
typedef int64_t( __fastcall* create_player_t )( void* ecx, void* edx, void* a3, double speed, unsigned int a5, unsigned int a6, int a7, int64_t a8, unsigned int a9, int64_t a10 );
create_player_t o_create_player;
int64_t __fastcall create_player( void* ecx, void* edx, void* a3, double speed, unsigned int a5, unsigned int a6, int a7, int64_t a8, unsigned int a9, int64_t a10 ) {
	speed *= 0.5;

	util::console::log( "[ app ] create_player( )\n" );

	return o_create_player( ecx, edx, a3, speed, a5, a6, a7, a8, a9, a10 );
}

bool hooks::initialize( ) {
	util::console::log( "[+] initializing hooks.\n" );

	HOOK( create_player, util::pattern_scan( "Spotify.exe", "E8 ? ? ? ? 48 81 C3 ? ? ? ? 48 3B D8" ).rel32( 0x1 ) );
	util::console::log( "[+] hooked create_player.\n" );

	return true;
}

void hooks::uninitialize( ) {
	UNHOOK( create_player );
}
