global idt_flush
idt_flush:
    lidt [rdi]
    ret
extern idt_handlers
%macro isr_stub_error 1
    global isr_stub_%1
    isr_stub_%1:
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
        push %1
        mov rax, %1
        mov rdi, rsp ; put value of stack pointer into paramter 1 of c interrupt handler

        mov rax, [idt_handlers + rax * 8]
        call rax
        add rsp, 8 ; skip int number
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
        add rsp, 8 ; skip error code
        iretq
%endmacro
%macro isr_stub 1
    global isr_stub_%1
    isr_stub_%1:
        push 0 ; fake error code lmao
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
        push %1
        mov rax, %1
        mov rdi, rsp ; put value of stack pointer into paramter 1 of c interrupt handler

        mov rax, [idt_handlers + rax * 8]
        call rax
        add rsp, 8 ; skip int number
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
        add rsp, 8 ; skip fake error code
        iretq
%endmacro

isr_stub 0
isr_stub_error 14
global isr_stub_32
isr_stub_32:
    push 0 ; fake error code lmao
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
    push 32
    mov rax, 32
    mov rdi, rsp ; put value of stack pointer into paramter 1 of c interrupt handler
    mov rax, [idt_handlers + rax * 8]
    call rax
    mov rsp, rax
    add rsp, 8 ; skip int number
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
    add rsp, 8 ; skip fake error code
    iretq