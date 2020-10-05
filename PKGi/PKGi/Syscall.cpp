#include "Syscall.h"

long syscall(long num, ...) {
	int64_t result = 0;                               // Storage for the result value.    
	__asm__(".intel_syntax noprefix\n"                // No need to shift anythign here, registers are already set up.
		"xor rax, rax\n"                              // We just need to make sure that rax, the register which holds the syscall number, holds 0 for syscall0.        
		"syscall"                                     // Now we just call the kernel and automaticly the correct registers are used.
		: "=a" (result)                               // Pipe return value in our function.
		::                                            // No input, no clobbering.
		);
	return result;                                    // Return the result to the caller.
}
