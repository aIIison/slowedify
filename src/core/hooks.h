#pragma once

#include <subhook/subhook.h>

namespace hooks {
	template < typename t >
	subhook_t create_hook( t& o, void* dst, void* src ) {
		auto hook = subhook_new( src, dst, ( subhook_flags_t ) ( SUBHOOK_64BIT_OFFSET | SUBHOOK_TRAMPOLINE ) );
		o         = reinterpret_cast< t >( subhook_get_trampoline( hook ) );
		subhook_install( hook );
		return hook;
	}

	__forceinline void destruct_hook( subhook_t hook ) {
		subhook_remove( hook );
		subhook_free( hook );
	}

	void  set_speed( float speed );
	float get_speed( );

	bool initialize( );
	void uninitialize( );
}  // namespace hooks
