// Linked into every test executable on Windows (see CMakeLists.txt).
//
// MSVC's assert() and abort() pop up modal dialogs by default. On a CI runner nobody clicks
// them, so a failing test would hang until the job times out. Route all reports to stderr and
// make the process exit with a failure code instead.
#ifdef _WIN32
#include <windows.h>
#include <crtdbg.h>
#include <cstdlib>
#include <cstdio>

namespace {
struct WinTestSetup {
    WinTestSetup() {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
        std::setvbuf(stdout, nullptr, _IONBF, 0);
    }
};
const WinTestSetup g_win_test_setup;
}  // namespace
#endif
