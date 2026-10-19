// CP12: the only translation unit of the mod that sees <windows.h> (kept out of mod.cpp so no Windows macro can collide with game headers).
// Both functions are ALREADY imported by the DLL (MSVC CRT start-up uses them), so the reviewed import set does not change; CI checks it.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
extern "C" unsigned long long devf_perf_qpc() noexcept {LARGE_INTEGER v;QueryPerformanceCounter(&v);return static_cast<unsigned long long>(v.QuadPart);}
extern "C" unsigned long long devf_perf_filetime() noexcept {
 FILETIME f;GetSystemTimeAsFileTime(&f);
 return (static_cast<unsigned long long>(f.dwHighDateTime)<<32)|f.dwLowDateTime;
}
