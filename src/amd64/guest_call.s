.text
.global amd64_core_call_guest
.global amd64_core_log_instruction
.extern amd64_core_log_instruction_impl

/*
  void amd64_core_call_guest(void *funcPtr, Cpu::State *state);
                              RDI ^                RSI ^

  We have to rescue: RBX, R12, R13, R14, R15
*/
amd64_core_call_guest:
  PUSH %rbp
  MOV %rsp, %rbp

  /* Rescue GPRs */
  PUSH %rbx
  PUSH %r12
  PUSH %r13
  PUSH %r14
  PUSH %r15

  /* Reset registers.  Not required, but eases debugging. */
  XOR %rax, %rax
  XOR %rcx, %rcx
  XOR %rbx, %rbx
  XOR %rdx, %rdx
  XOR %r8, %r8
  XOR %r9, %r9
  XOR %r10, %r10
  XOR %r11, %r11
  XOR %r12, %r12
  XOR %r13, %r13
  XOR %r14, %r14
  XOR %r15, %r15

  /* Set up guest frame. */
  MOV 0(%rsi), %bl   /* A */
  MOV 1(%rsi), %bh   /* X */
  MOV 2(%rsi), %r12b /* Y */
  MOV 3(%rsi), %r13b /* S */
  MOV 4(%rsi), %r14b /* P */
  MOV 5(%rsi), %r15d /* Cycles */

  /* Call guest function! */
  PUSH %rsi
  CALL *%rdi
  POP  %rsi

  /* Rescue guest frame into state structure. */
  MOV %bl, 0(%rsi)   /* A */
  MOV %bh, 1(%rsi)   /* X */
  MOV %r12b, 2(%rsi) /* Y */
  MOV %r13b, 3(%rsi) /* S */
  MOV %r14b, 4(%rsi) /* P */
  MOV %r15d, 5(%rsi) /* Cycles */

  /* Also update `pc` and `reason` */
  MOV %cx, 9(%rsi)   /* PC */
  MOV %al, 11(%rsi)  /* Reason */

  /* Restore GPRs */
  POP %r15
  POP %r14
  POP %r13
  POP %r12
  POP %rbx

  /* Done! */
  MOV %rbp, %rsp
  POP %rbp
  RET

/*
  void amd64_core_log_instruction(Cpu::State *state, uint8_t cmd, uint8_t mode, uint16_t op);
*/
amd64_core_log_instruction:
  PUSH %rbp
  MOV %rsp, %rbp

  MOV %bl, 0(%rdi)   /* A */
  MOV %bh, 1(%rdi)   /* X */
  MOV %r12b, 2(%rdi) /* Y */
  MOV %r13b, 3(%rdi) /* S */
  MOV %r14b, 4(%rdi) /* P */
  MOV %r15d, 5(%rdi) /* Cycles */
  MOV %cx, 9(%rdi)   /* PC */

  PUSH %rbx
  PUSH %rcx
  PUSH %r12
  PUSH %r13
  PUSH %r14
  PUSH %r15

  MOV %r8w, %cx /* CX was the PC, now it's the operand */
  CALL amd64_core_log_instruction_impl

  POP %r15
  POP %r14
  POP %r13
  POP %r12
  POP %rcx
  POP %rbx

  MOV %rbp, %rsp
  POP %rbp
  RET
