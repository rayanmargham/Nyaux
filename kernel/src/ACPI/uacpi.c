#include "acpi.h"
#include "main.h"
#include "drivers/serial.h"
#include "vmm.h"
#include "pmm.h"
#include "bump.h"
#include <uacpi/kernel_api.h>
#include <drivers/apic.h>
static bool isPower2(uint64_t num)
{
	auto firstBitPos = __builtin_ctzll(num);
	num &= ~(1<<firstBitPos);
	return num == 0 /* A power of two only ever has one bit set. */;
}
uacpi_status uacpi_kernel_raw_memory_read(
    uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value
)
{
    void *virt = address + hhdm_request.response->offset;
    switch (byte_width)
    {
        case 1: *out_value = *(uint8_t*)virt; break;
        case 2: *out_value = *(uint16_t*)virt; break;
		case 4: *out_value = *(uint32_t*)virt; break;
		case 8: *out_value = *(uint64_t*)virt; break;
		default: return UACPI_STATUS_INVALID_ARGUMENT;
    }
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_memory_write(
    uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 in_value
)
{
    void *virt = address + hhdm_request.response->offset;
    switch (byte_width)
	{
		case 1: *( uint8_t*)virt = in_value; break;
		case 2: *(uint16_t*)virt = in_value; break;
		case 4: *(uint32_t*)virt = in_value; break;
		case 8: *(uint64_t*)virt = in_value; break;
		default: return UACPI_STATUS_INVALID_ARGUMENT;
	}
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_read(
    uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value
)
{
    if (!isPower2(byte_width))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (byte_width > 8)
        return UACPI_STATUS_INVALID_ARGUMENT;
    switch (byte_width)
    {
        case 1: *out_value = inb(address); break;
        case 2: *out_value = inw(address); break;
        case 4: *out_value = ind(address); break;
        case 8: return UACPI_STATUS_INVALID_ARGUMENT; break;
    }
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_io_write(
    uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64 in_value
)
{
    if (!isPower2(byte_width))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (byte_width > 8)
        return UACPI_STATUS_INVALID_ARGUMENT;
        switch (byte_width)
    {
        case 1: outb(address, in_value); break;
        case 2: outw(address, in_value); break;
        case 4: outd(address, in_value); break;
        case 8: return UACPI_STATUS_INVALID_ARGUMENT; break;
    }
    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read(
    uacpi_pci_address *address, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_pci_write(
    uacpi_pci_address *address, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
struct io_range
{
	uacpi_io_addr base;
	uacpi_size len;
};
uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
)
{
    struct io_range *rng = kmalloc(sizeof(struct io_range));
    rng->base = base;
    rng->len = len;
    *out_handle = (uacpi_handle)rng;
    return UACPI_STATUS_OK;
}
void uacpi_kernel_io_unmap(uacpi_handle handle)
{
    kfree((struct io_range*)handle, sizeof(struct io_range));
}

uacpi_status uacpi_kernel_io_read(
    uacpi_handle hnd, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
)
{
    struct io_range *rng = (struct io_range*)hnd;
    if (offset >= rng->len)
    {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    return uacpi_kernel_raw_io_read(rng->base + offset, byte_width, value);
}
uacpi_status uacpi_kernel_io_write(
    uacpi_handle hnd, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
)
{
    struct io_range *rng = (struct io_range*)hnd;
    if (offset >= rng->len)
    {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    return uacpi_kernel_raw_io_write(rng->base + offset, byte_width, value);
}
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    return ((uint64_t)addr + hhdm_request.response->offset);
}
void uacpi_kernel_unmap(void *addr, uacpi_size len)
{
    // nothing
}

void *uacpi_kernel_alloc(uacpi_size size)
{
    return kmalloc(size);

}
void *uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
    void *man = kmalloc(count * size);
    memset(man, 0, count * size);
    return man;
}
void uacpi_kernel_free(void *mem, uacpi_size size_hint)
{
    kfree(mem, size_hint);
}
void uacpi_kernel_log(enum uacpi_log_level level, const uacpi_char* meow, ...)
{
    va_list list;
    va_start(list, meow);
    uacpi_kernel_vlog(level, meow, list);
    va_end(list);
}
void uacpi_kernel_vlog(enum uacpi_log_level level, const uacpi_char* format, uacpi_va_list list)
{
    const char* prefix = "UNKNOWN";
		switch (level)
		{
		case UACPI_LOG_TRACE:
			prefix = "TRACE";
			break;
		case UACPI_LOG_INFO:
			prefix = "INFO";
			break;
		case UACPI_LOG_WARN:
			prefix = "WARN";
			break;
		case UACPI_LOG_ERROR:
			prefix = "ERROR";
			break;
		default:
			break;
		}
    char buff[128];
    npf_vsnprintf(buff, 128, format, list);
    serial_print(buff);
}
uacpi_u64 uacpi_kernel_get_ticks(void)
{
    return 46644; // MAGIC NUMBER VERY FUNNY !!!
}
void uacpi_kernel_stall(uacpi_u8 usec)
{
    ksleep(usec * 1000);
}
void uacpi_kernel_sleep(uacpi_u64 msec)
{
    // WOULD BLOCK THREAD BUT SCHEDULER DOESNT HAVE THAT ATM LOL
    // SET CUR THREAD STATE TO BLOCKED
}
uacpi_handle uacpi_kernel_create_mutex(void)
{
    return 1;
}
void uacpi_kernel_free_mutex(uacpi_handle)
{

}
uacpi_handle uacpi_kernel_create_event(void)
{
    return 1;
}
void uacpi_kernel_free_event(uacpi_handle)
{

}
uacpi_bool uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16)
{
    return UACPI_TRUE;
}
void uacpi_kernel_release_mutex(uacpi_handle)
{

}
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16)
{
    return UACPI_TRUE;
}
void uacpi_kernel_signal_event(uacpi_handle)
{
    
}
void uacpi_kernel_reset_event(uacpi_handle)
{

}
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler, uacpi_handle irq_handle
)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_handle uacpi_kernel_create_spinlock(void)
{
    return 1;
}
uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle)
{
    return 1;
}
void uacpi_kernel_spinlock_unlock(uacpi_handle, uacpi_cpu_flags)
{
    
}
uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_wait_for_work_completion(void)
{
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_thread_id uacpi_kernel_get_thread_id(void)
{
	return 0; //fuck u
}
