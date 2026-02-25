#include <iostream>
#include <string>

// clang-format off
#include <windows.h>
#include <tlhelp32.h>
// clang-format on

DWORD get_pid( const wchar_t* name ) {
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof( PROCESSENTRY32W );

	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( snapshot == INVALID_HANDLE_VALUE )
		return 0;

	if ( Process32FirstW( snapshot, &entry ) ) {
		do {
			if ( _wcsicmp( entry.szExeFile, name ) == 0 ) {
				CloseHandle( snapshot );
				return entry.th32ProcessID;
			}
		} while ( Process32NextW( snapshot, &entry ) );
	}

	CloseHandle( snapshot );
	return 0;
}

bool inject( DWORD pid, const char* dll ) {
	HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
	if ( !process ) {
		printf( "[!] Failed to open process.\n" );
		return false;
	}

	size_t path_len   = strlen( dll ) + 1;
	void*  remote_mem = VirtualAllocEx( process, NULL, path_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if ( !remote_mem ) {
		printf( "[!] failed to allocate memory.\n" );
		CloseHandle( process );
		return false;
	}

	if ( !WriteProcessMemory( process, remote_mem, dll, path_len, NULL ) ) {
		printf( "[!] failed to write memory.\n" );
		VirtualFreeEx( process, remote_mem, 0, MEM_RELEASE );
		CloseHandle( process );
		return false;
	}

	HMODULE kernel32     = GetModuleHandleA( "kernel32.dll" );
	FARPROC load_library = GetProcAddress( kernel32, "LoadLibraryA" );

	HANDLE thread = CreateRemoteThread( process, NULL, 0, ( LPTHREAD_START_ROUTINE ) load_library, remote_mem, 0, NULL );
	if ( !thread ) {
		printf( "[!] failed to create remote thread.\n" );
		VirtualFreeEx( process, remote_mem, 0, MEM_RELEASE );
		CloseHandle( process );
		return false;
	}

	WaitForSingleObject( thread, INFINITE );

	DWORD exit_code;
	GetExitCodeThread( thread, &exit_code );

	VirtualFreeEx( process, remote_mem, 0, MEM_RELEASE );
	CloseHandle( thread );
	CloseHandle( process );

	return exit_code != 0;
}

int main( ) {
	printf( "[+] waiting for spotify.exe...\n" );

	DWORD pid = 0;
	while ( !pid ) {
		pid = get_pid( L"spotify.exe" );
		if ( !pid )
			Sleep( 1000 );
	}

	printf( "[+] found spotify.exe, pid: %lu.\n", pid );
	printf( "[+] injecting...\n" );

	if ( inject( pid, "slowedify.dll" ) ) {
		printf( "[+] injection successful!\n" );
	} else {
		printf( "[!] injection failed!\n" );
		std::cin.get( );
		return 1;
	}

	return 0;
}
