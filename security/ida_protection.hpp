#pragma once
#include <Windows.h>
#include <winternl.h>
#include <intrin.h>

namespace security {

class ida_protection {
public:
    static constexpr uint32_t IDA_SIGNATURE = 0x44613949; // "IDA" signature

    static void corrupt_ida_database() {
        // Write random data to memory to corrupt potential IDA analysis
        unsigned char* ptr = (unsigned char*)GetModuleHandle(NULL);
        SIZE_T size = 1024 * 1024; // 1MB
        DWORD oldProtect;
        
        if(VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            for(SIZE_T i = 0; i < size; i += 4) {
                *(uint32_t*)(ptr + i) ^= __rdtsc() ^ IDA_SIGNATURE;
            }
            VirtualProtect(ptr, size, oldProtect, &oldProtect);
        }
    }

    static bool check_debugger() {
        __try {
            return IsDebuggerPresent() || CheckRemoteDebuggerPresent(GetCurrentProcess(), nullptr);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return true;
        }
    }

    static bool check_hardware_breakpoints() {
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if(GetThreadContext(GetCurrentThread(), &ctx)) {
            return ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0;
        }
        return false;
    }

    static bool check_timing() {
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        
        // Some complex operation that should be very fast
        volatile int x = 0;
        for(int i = 0; i < 1000; i++) x += i;
        
        QueryPerformanceCounter(&end);
        double elapsed = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        
        return elapsed > 10.0; // If it takes more than 10ms, likely being debugged
    }

    static void initialize() {
        // Anti-attach protection
        AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS ex) -> LONG {
            if(ex->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT ||
               ex->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP) {
                corrupt_ida_database();
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        });
    }

    static bool check_for_ida() {
        if(check_debugger() || check_hardware_breakpoints() || check_timing()) {
            corrupt_ida_database();
            return true;
        }
        return false;
    }

    static void protect() {
        static volatile bool is_protected = false;
        if(!is_protected) {
            initialize();
            is_protected = true;
        }
        
        if(check_for_ida()) {
            ExitProcess(0);
        }
    }
};

// Anti-disassembly macros - now using standard C++ try-catch instead of SEH
#define ANTI_IDA_BEGIN try {
#define ANTI_IDA_END } catch(...) { security::ida_protection::corrupt_ida_database(); }

// Use this to protect important functions
#define PROTECT_FUNCTION() \
    static volatile bool is_checking = true; \
    if(is_checking) { \
        is_checking = false; \
        security::ida_protection::check_for_ida(); \
        is_checking = true; \
    }

} // namespace security 