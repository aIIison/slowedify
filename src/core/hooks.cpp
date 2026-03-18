#include "hooks.h"

#include "util/util.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

// carry-over from the previous callback — at most a few frames.
static std::vector< float > g_carry;
static float                g_read_pos = 0.0f;
static std::mutex           g_mutex;
std::atomic< float >        g_speed = 1.0f;

struct audio_buffer_t {
	float*   data;
	uint64_t size;
};

void* g_audio_source = nullptr;
void( __fastcall* o_audio_source_read )( int64_t, uint64_t*, audio_buffer_t*, uint64_t );
void __fastcall audio_source_read( int64_t ecx, uint64_t* n_samples, audio_buffer_t* buffer, uint64_t flags ) {
	o_audio_source_read( ecx, n_samples, buffer, flags );

	const float speed = g_speed.load( );
	if ( speed == 1.0f || !buffer || !buffer->data || !n_samples || *n_samples == 0 )
		return;

	std::lock_guard< std::mutex > lock( g_mutex );

	float*         samples    = buffer->data;
	const uint64_t num_frames = *n_samples / 2;  // stereo

	// build working buffer = leftover carry from last callback + fresh incoming samples.
	// carry is at most ceil(speed) * 2 samples, so this is always small.
	const size_t         incoming = *n_samples;
	std::vector< float > src;
	src.reserve( g_carry.size( ) + incoming );
	src.insert( src.end( ), g_carry.begin( ), g_carry.end( ) );
	src.insert( src.end( ), samples, samples + incoming );

	// resample src -> samples in-place at `speed` frames per output frame.
	uint64_t out_frame = 0;
	while ( out_frame < num_frames ) {
		const uint64_t s0   = ( uint64_t ) g_read_pos;
		const float    frac = g_read_pos - ( float ) s0;
		const uint64_t i0   = s0 * 2;
		const uint64_t i1   = i0 + 2;

		if ( i1 + 1 >= src.size( ) )
			break;

		// linear interpolation between adjacent stereo frames.
		samples[ out_frame * 2 ]     = src[ i0 ] + ( src[ i1 ] - src[ i0 ] ) * frac;
		samples[ out_frame * 2 + 1 ] = src[ i0 + 1 ] + ( src[ i1 + 1 ] - src[ i0 + 1 ] ) * frac;

		out_frame++;
		g_read_pos += speed;
	}

	// save whatever src frames we didn't consume as carry for next callback.
	// this is typically 0-2 frames at normal speeds.
	const uint64_t consumed = ( uint64_t ) g_read_pos;
	const size_t   used     = consumed * 2;
	if ( used < src.size( ) )
		g_carry.assign( src.begin( ) + used, src.end( ) );
	else
		g_carry.clear( );

	// only keep the fractional part of g_read_pos, offset by consumed frames.
	g_read_pos -= ( float ) consumed;
	if ( g_read_pos < 0.f )
		g_read_pos = 0.f;

	// hold last sample for any frames we couldn't fill (buffer ran dry mid-callback).
	if ( out_frame < num_frames ) {
		const float last_l = out_frame > 0 ? samples[ ( out_frame - 1 ) * 2 ] : 0.f;
		const float last_r = out_frame > 0 ? samples[ ( out_frame - 1 ) * 2 + 1 ] : 0.f;
		for ( uint64_t i = out_frame; i < num_frames; i++ ) {
			samples[ i * 2 ]     = last_l;
			samples[ i * 2 + 1 ] = last_r;
		}
	}

	*n_samples = num_frames * 2;
}

subhook_t wasapi_renderer_read_h;
int64_t( __fastcall* o_wasapi_renderer_read )( address_t, int64_t, char );
int64_t __fastcall wasapi_renderer_read( address_t ecx, int64_t a2, char a3 ) {
	static bool hooked = false;
	if ( !hooked && ecx ) {
		g_audio_source = ecx.at( 5520ull );
		if ( g_audio_source ) {
			o_audio_source_read = util::hook_virtual< 1 >( g_audio_source, &audio_source_read );

			util::console::log( "[+] hooked " PRINT_YELLOW "audio_source::read" PRINT_CYAN " [%p]" PRINT_RESET ".\n", o_audio_source_read );
			hooked = true;
		}
	}
	return o_wasapi_renderer_read( ecx, a2, a3 );
}

void hooks::set_speed( float speed ) {
	g_speed = std::clamp( speed, 0.1f, 3.0f );
	// clear carry on speed change to avoid interpolating across a speed boundary.
	std::lock_guard< std::mutex > lock( g_mutex );
	g_carry.clear( );
	g_read_pos = 0.f;
}

float hooks::get_speed( ) {
	return g_speed.load( );
}

bool hooks::initialize( ) {
	util::console::log( "[+] initializing hooks.\n" );

	auto addr = util::pattern_scan(
					"spotify.dll",
					"48 89 5c 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8d 6c 24 ? 48 81 ec ? ? ? ? 48 8b 05 ? ? ? ? 48 33 c4 48 89 45 ? 44 88 44 24 ? 4c 8b fa 48 8b f9" );
	wasapi_renderer_read_h = create_hook( o_wasapi_renderer_read, wasapi_renderer_read, addr );
	util::console::log( "[+] hooked " PRINT_YELLOW "spotify::playback::WasapiRenderer::read" PRINT_CYAN " [%p]" PRINT_RESET ".\n", addr );

	return true;
}

void hooks::uninitialize( ) {
	if ( g_audio_source && o_audio_source_read )
		util::hook_virtual< 1 >( g_audio_source, o_audio_source_read );

	destruct_hook( wasapi_renderer_read_h );
}
