#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef SYSCALL_H
#define SYSCALL_H

long syscall(long num, ...);

static inline __attribute__((always_inline)) uint64_t readmsr(uint32_t __register) {
	uint32_t __edx, __eax;

	__asm__ volatile (
		"rdmsr"
		: "=d"(__edx),
		"=a"(__eax)
		: "c"(__register)
		);

	return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}

static inline __attribute__((always_inline)) uint64_t readcr0(void) {
	uint64_t cr0;

	__asm__ volatile (
		"movq %0, %%cr0"
		: "=r" (cr0)
		: : "memory"
		);

	return cr0;
}
static inline __attribute__((always_inline)) void writecr0(uint64_t cr0) {
	__asm__ volatile (
		"movq %%cr0, %0"
		: : "r" (cr0)
		: "memory"
		);
}

#endif