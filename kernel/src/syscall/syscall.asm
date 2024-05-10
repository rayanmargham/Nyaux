global syscall_handler
extern syscall_handlers
syscall_handler:
    swapgs
    mov [gs:8], rsp ; save user stack
    mov rsp, [gs:0] ; load kernel stack
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    mov rsi, [gs:16]
    mov rax, [syscall_handlers + rax * 8]
    call rax
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    mov rsp, [gs:8]
    swapgs
    o64 sysret