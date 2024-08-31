#pragma once

#include <subhook/subhook.h>

#define HOOK( name, src ) create_hook( src, name, reinterpret_cast< void** >( &o_##name ), &name##_hook )
#define UNHOOK( name )    destruct_hook( name##_hook )

namespace hooks {
	__forceinline void create_hook( void* src, void* dst, void** o, subhook_t* hook ) {
		*hook = subhook_new( src, dst, ( subhook_flags_t ) ( SUBHOOK_64BIT_OFFSET | SUBHOOK_TRAMPOLINE ) );
		*o    = reinterpret_cast< decltype( o ) >( subhook_get_trampoline( *hook ) );
		subhook_install( *hook );
	}
	__forceinline void destruct_hook( subhook_t hook ) {
		subhook_remove( hook );
		subhook_free( hook );
	}

	bool initialize( );
	void uninitialize( );
}  // namespace hooks
