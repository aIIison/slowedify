#include "hooks.h"

#include "util/util.h"

#include <atomic>
#include <deque>

std::deque< float >  g_sample_buffer;
std::atomic< float > g_speed    = 1.0f;
float                g_read_pos = 0.0f;

struct audio_buffer_t {
	float*   data;
	uint64_t size;
};

void* g_audio_source = nullptr;
void( __fastcall* o_audio_source_read )( int64_t ecx, uint64_t* n_samples, audio_buffer_t* buffer, uint64_t flags );
void __fastcall audio_source_read( int64_t ecx, uint64_t* n_samples, audio_buffer_t* buffer, uint64_t flags ) {
	o_audio_source_read( ecx, n_samples, buffer, flags );

	float speed = g_speed.load( );
	if ( speed == 1.0f || !buffer || !buffer->data || !n_samples || *n_samples == 0 )
		return;

	float*   samples    = buffer->data;
	uint64_t num_frames = *n_samples / 2;

	// push incoming samples to buffer.
	for ( uint64_t i = 0; i < *n_samples; i++ )
		g_sample_buffer.push_back( samples[ i ] );

	// resample from buffer.
	uint64_t out_frame = 0;
	while ( out_frame < num_frames ) {
		uint64_t src_frame = ( uint64_t ) g_read_pos;
		float    frac      = g_read_pos - ( float ) src_frame;
		uint64_t src_idx   = src_frame * 2;

		if ( src_idx + 3 >= g_sample_buffer.size( ) )
			break;

		// lerp stereo.
		samples[ out_frame * 2 ]     = g_sample_buffer[ src_idx ] + ( g_sample_buffer[ src_idx + 2 ] - g_sample_buffer[ src_idx ] ) * frac;
		samples[ out_frame * 2 + 1 ] = g_sample_buffer[ src_idx + 1 ] + ( g_sample_buffer[ src_idx + 3 ] - g_sample_buffer[ src_idx + 1 ] ) * frac;

		out_frame++;
		g_read_pos += speed;
	}

	// consume processed samples.
	uint64_t consumed = ( uint64_t ) g_read_pos;
	if ( consumed * 2 <= g_sample_buffer.size( ) ) {
		g_sample_buffer.erase( g_sample_buffer.begin( ), g_sample_buffer.begin( ) + consumed * 2 );
		g_read_pos -= ( float ) consumed;
	}

	// fill any gap with last sample.
	if ( out_frame > 0 && out_frame < num_frames ) {
		float last_l = samples[ ( out_frame - 1 ) * 2 ];
		float last_r = samples[ ( out_frame - 1 ) * 2 + 1 ];
		for ( uint64_t i = out_frame; i < num_frames; i++ ) {
			samples[ i * 2 ]     = last_l;
			samples[ i * 2 + 1 ] = last_r;
		}
	}

	// prevent buffer overflow.
	if ( g_sample_buffer.size( ) > 96000 )
		g_sample_buffer.erase( g_sample_buffer.begin( ), g_sample_buffer.begin( ) + 48000 );
}

subhook_t wasapi_renderer_read_h;
int64_t( __fastcall* o_wasapi_renderer_read )( address_t ecx, int64_t a2, char a3 );
int64_t __fastcall wasapi_renderer_read( address_t ecx, int64_t a2, char a3 ) {
	static bool hooked = false;

	if ( !hooked && ecx ) {
		g_audio_source = ecx.at( 5440ull );

		if ( g_audio_source ) {
			o_audio_source_read = util::hook_virtual< 1 >( g_audio_source, &audio_source_read );

			util::console::log( "[+] hooked " PRINT_YELLOW "audio_source::read" PRINT_CYAN " [%p]" PRINT_RESET ".\n", o_audio_source_read );
			hooked = true;
		}
	}

	return o_wasapi_renderer_read( ecx, a2, a3 );
}

void hooks::set_speed( float speed ) {
	speed      = std::max( 0.1f, std::min( 3.0f, speed ) );
	g_speed    = speed;
	g_read_pos = 0.0f;
	g_sample_buffer.clear( );
}

float hooks::get_speed( ) {
	return g_speed.load( );
}

bool hooks::initialize( ) {
	util::console::log( "[+] initializing hooks.\n" );

	auto wasapi_renderer_read_addr = util::pattern_scan(
					"spotify.dll",
					"48 89 5c 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8d 6c 24 ? 48 81 ec ? ? ? ? 48 8b 05 ? ? ? ? 48 33 c4 48 89 45 ? 45 8a e8 4c 8b fa 48 8b f9" );

	wasapi_renderer_read_h = create_hook( o_wasapi_renderer_read, wasapi_renderer_read, wasapi_renderer_read_addr );

	util::console::log( "[+] hooked " PRINT_YELLOW "spotify::playback::WasapiRenderer::read" PRINT_CYAN " [%p]" PRINT_RESET ".\n", wasapi_renderer_read_addr );

	return true;
}

void hooks::uninitialize( ) {
	if ( g_audio_source && o_audio_source_read ) {
		util::hook_virtual< 1 >( g_audio_source, o_audio_source_read );
	}

	destruct_hook( wasapi_renderer_read_h );
}
