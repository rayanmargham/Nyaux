#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "flanterm/flanterm.h"
#include "flanterm/backends/fb.h"

void write_color(struct flanterm_context *ctx, char *buf, int type);
void write(struct flanterm_context *ctx, char *buf);
char *itoa(char *buffer, size_t number);
char *itoah(char *buffer, size_t number);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int strncmp(s1, s2, n);
void kprintf(const char* format, ...);
extern struct flanterm_context *ctx;