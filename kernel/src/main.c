#include "main.h"
#include "gdt.h"
#include "drivers/serial.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "ACPI/acpi.h"
#include <uacpi/types.h>
#include "sched/sched.h"
#include "bump.h"
#include "drivers/apic.h"
#include "drivers/ps2.h"
#include "fs/vfs.h"
#include "fs/tar.h"
#include <syscall/syscall.h>
#include <fpu/xsave.h>
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_IMPLEMENTATION
#include <lib/nanoprintf.h>
#include <lib/kpanic.h>
// Set the base revision to 1, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
struct flanterm_context *ctx;
static volatile LIMINE_BASE_REVISION(1);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once.

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 1
};
struct limine_module_request module_request = 
{
    .id = LIMINE_MODULE_REQUEST,
    .revision = 1
};
// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}
size_t strlen(const char *str) {
    const char *s;

    for (s = str; *s; s++);
    return (s - str);
}
size_t ilog10(size_t number) {
  size_t x = 0;

  if (number == 0) return 0;

  while (number) {
    number /= 10;
    x++;
  }

  return x - 1;
}
size_t ilog16(size_t number) {
  size_t x = 0;

  if (number == 0) return 0;

  while (number) {
    number /= 16;
    x++;
  }

  return x - 1;
}
char digits[] = {
        '0', '1', '2', '3', '4',
        '5', '6', '7', '8', '9',
};
char digitsh[] = {
        '0', '1', '2', '3', '4',
        '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

char *itoa(char *buffer, size_t number) {
  size_t i;

  if (number == 0) {
    memcpy(buffer, "0", 2);
    return buffer;
  }

  const size_t digitcnt = ilog10(number);

  for (i = 0; i < digitcnt + 1; i++) {
    size_t digit = number % 10;
    char c = digits[digit];
    buffer[digitcnt - i] = c;
    number /= 10;
  }

  buffer[i] = '\0';
  return buffer;
}
char *itoah(char *buffer, size_t number) {
  size_t i;

  if (number == 0) {
    memcpy(buffer, "0", 2);
    return buffer;
  }

  const size_t digitcnt = ilog16(number);

  for (i = 0; i < digitcnt + 1; i++) {
    size_t digit = number % 16;
    char c = digitsh[digit];
    buffer[digitcnt - i] = c;
    number /= 16;
  }

  buffer[i] = '\0';
  return buffer;
}
// Halt and catch fire function.
static void hcf(void) {
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
void write(struct flanterm_context *ctx, char *buf)
{
    flanterm_write(ctx, buf, strlen(buf));
}
size_t strnlen(const char *s, size_t maxlen)
{
	size_t len;

	for (len = 0; len < maxlen; len++, s++) {
		if (!*s)
			break;
	}
	return (len);
}
int strncmp(const char* a, const char* b, size_t len) {
    for (; len; --len, ++a, ++b) {
        int ret = *a - *b;
        if (ret != 0 || !*a || !*b) {
            return ret;
        }
    }
    return 0;
}
int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (0);
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}
void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}
char *strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from); ++from, ++to);
	return(save);
}
void write_color(struct flanterm_context *ctx, char *buf, int type)
{
    switch (type)
    {
    case 0: // INFO
        write(ctx, "[ ");
        ctx->set_text_fg_rgb(ctx, 0xbae1ff);
        write(ctx, "INFO");
        ctx->set_text_fg_default(ctx);
        write(ctx, " ]");
        write(ctx, " ");
        write(ctx, buf);
        break;
    case 1: // STATUS OKAY
        write(ctx, "[ ");
        ctx->set_text_fg_rgb(ctx, 0x00FF00);
        write(ctx, "OK");
        ctx->set_text_fg_default(ctx);
        write(ctx, " ]");
        write(ctx, " ");
        write(ctx, buf);

        break;
    case 2: // WARNING
        write(ctx, "[ ");
        ctx->set_text_fg_rgb(ctx, 0xFDFD96);
        write(ctx, "WARNING");
        ctx->set_text_fg_default(ctx);
        write(ctx, " ]");
        write(ctx, " ");
        write(ctx, buf);
        break;
    case 3: // ERROR
        write(ctx, "[ ");
        ctx->set_text_fg_rgb(ctx, 0xFF6961);
        write(ctx, "ERROR");
        ctx->set_text_fg_default(ctx);
        write(ctx, " ]");
        write(ctx, " ");
        write(ctx, buf);
        break;
    case 4: // KPANIC
        write(ctx, "[ ");
        ctx->set_text_fg_rgb(ctx, 0xFF6961);
        write(ctx, "NYA KPANIC");
        ctx->set_text_fg_default(ctx);
        write(ctx, " ]");
        write(ctx, " ");
        ctx->set_text_bg_rgb(ctx, 0xFF6961);
        write(ctx, buf);
        ctx->set_text_bg_default(ctx);
    default:
        break;
    }
}
void kputc(int ch, void*)
{
    if (ctx != NULL)
    {
        char c = ch;
        flanterm_write(ctx, &c, 1);
        print_charserial(ch);
    }
}
UACPI_PRINTF_DECL(1, 2)
void kprintf(const char* format, ...)
{
    va_list args;
    va_start (args, format);
    npf_vpprintf(kputc, NULL, format, args);
    
    va_end(args);
}


// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void _start(void) {
    uint32_t bg = 0x000000;
    uint32_t fg = 0xf0faf6;
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf(); 
    }
    // for(int y = 0; y <  768; y++) {
    //     for(int x = 0; x < 1024; x++) {
    //         if (fb[x + y * framebuffer->width] == 0x000000)
    //         {
    //             fb[x + y * framebuffer->width] = img[x + y * 1024];
    //         }
            
    //     }
    // }
    // Fetch the first framebuffer.
    
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    // volatile uint32_t *fb = framebuffer->address;
    init_gdt();
    ctx = flanterm_fb_init(NULL, NULL, framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch, framebuffer->red_mask_size, framebuffer->red_mask_shift, framebuffer->green_mask_size, framebuffer->green_mask_shift, framebuffer->blue_mask_size, framebuffer->blue_mask_shift, NULL, NULL, NULL, &bg, &fg, NULL, NULL, NULL, 0, 0, 1, 0, 0, 0);
    write(ctx, "Nya Kernel Loading...\n");
    write_color(ctx, "GDT Loaded!\n", 1);
    init_serial();
    init_idt();
    write_color(ctx, "IDT Initialized!\n", 1);
    pmm_init();
    write_color(ctx, "PMM Initialized!\n", 1);
    vmm_init();
    acpi_init();
    write(ctx, "\e[mAmazing!\n");
    kprintf("PrintF Test: %s\n", "meow");
    apic_init();
    ps2_init();
    test_vfs();
    parse_tar_and_populate_tmpfs(module_request.response->modules[0]);
    uint64_t syscall = 0;
    readmsr(0xC0000080, &syscall);
    kprintf("syscall msr : %p\n",(void*)syscall);
    kprintf("Enabling syscall extention...\n");
    syscall |= 1;
    writemsr(0xC0000080, syscall);
    uint64_t star = 0;
    readmsr(0xC0000081, &star);
    star |= ((uint64_t)0x28 << 32); // kernel cs, not segment selector but offset in gdt
    star |= ((uint64_t)0x30 << 48); // user cs, same thing
    writemsr(0xC0000081, star);
    writemsr(0xC0000082, (uint64_t)syscall_handler);
    writemsr(0xC0000084, (1 << 9));
    kprintf("Star MSR %p\n", (void*)star);
    syscall_init();
    ts.rsp0 = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096);
    kprintf("rsp0: %p\n", (void*)ts.rsp0);
    uint64_t cr0 = (uint64_t)read_cr0();
    cr0 |= (1 << 1);
    cr0 = cr0 & ~(0 << 2);
    update_cr0(cr0);
    uint64_t cr4 = (uint64_t)read_cr4();
    cr4 |= (1 << 9);
    cr4 |= (1 << 10);
    update_cr4(cr4);
    setup_xsave();
    write_color(ctx, "Syscalls Enabled, SSE and XSave Enabled, Entering Scheduler...\n", 1);
    // we have just enabled sse
    sched_init();
    for (;;) {
        asm("hlt");
    }
    // We're done, just hang...
    hcf();
}
