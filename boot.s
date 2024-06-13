
.macro salva_contexto
  push {r0}
  ldr r0, =user_regs
  stmib r0, {r1-r14}^        // registradores do usuário
  str lr, [r0, #60]          // endereço de retorno (pc)
  mrs r1, spsr
  str r1, [r0, #164]         // cpsr
  pop {r1}
  str r1, [r0]               // salva o r0 original
.endm

.section .init
.global start
start:
  /*
   * Vetor de interrupções
   * Deve ser copiado no enderço 0x0000
   */
  ldr pc, _reset
  ldr pc, _undef
  ldr pc, _swi
  ldr pc, _iabort
  ldr pc, _dabort
  nop
  ldr pc, _irq
  ldr pc, _fiq

  _reset:    .word   reset
  _undef:    .word   undef
  _swi:      .word   swi
  _iabort:   .word   iabort
  _dabort:   .word   dabort
  _irq:      .word   irq
  _fiq:      .word   irq

/*
 * Instrução inicial (vetor de reset).
 */
.text
reset:
.if RPICPU == 2
  /*
   * Verifica priviégio de execução EL2 (HYP) ou EL1 (SVC)
   * Somente para Raspberry Pi 2 e 3
   */
  mrs r0, cpsr
  and r0, r0, #0x1f
  cmp r0, #0x1a
  bne continua

  /*
   * Sai do modo EL2 (HYP)
   */
  mrs r0, cpsr
  bic r0, r0, #0x1f
  orr r0, r0, #0x13
  msr spsr_cxsf, r0
  add lr, pc, #4       // aponta o rótulo 'continua'
  msr ELR_hyp, lr
  eret                 // 'retorna' do privilégio EL2 para o EL1

continua:
  /*
   * Verifica o índice das CPUs
   */
  mrc p15,0,r0,c0,c0,5    // registrador MPIDR
  ands r0, r0, #0xff
  beq core0
  b trava

// Execução do núcleo #0
.endif
core0:
  /*
   * configura os stack pointers
   */
  mov r0, #0xd2     // Modo IRQ
  msr cpsr_c,r0
  ldr sp, =stack_irq

  mov r0, #0xd3     // Modo SVC
  msr cpsr_c,r0
  ldr sp, =stack_svr

  // Continua executando no modo supervisor (SVC), interrupções desabilitadas

  /*
   * Move o vetor de interrupções para o endereço 0
   */
  ldr r0, =load_addr
  mov r1, #0x0000
  ldmia r0!, {r2,r3,r4,r5,r6,r7,r8,r9}
  stmia r1!, {r2,r3,r4,r5,r6,r7,r8,r9}
  ldmia r0!, {r2,r3,r4,r5,r6,r7,r8,r9}
  stmia r1!, {r2,r3,r4,r5,r6,r7,r8,r9}

   /*
    * Zera segmento BSS
    */
   ldr r0, =bss_begin
   ldr r1, =bss_end
   mov r2, #0
loop_bss:
   cmp r0, r1
   bge done_bss
   strb r2, [r0], #1
   b loop_bss

done_bss:
  /*
   * Executa a função main
   */
  b main

/*
 * Tratamento dos eventos
 */
undef:
  salva_contexto
  mov r0, #0x04      // SIG_ILL
  b goto_gdb
iabort:
  sub lr, lr, #4
  salva_contexto
  mov r0, #0x06      // SIG_ABRT
  b goto_gdb
dabort:
  sub lr, lr, #8
  salva_contexto
  mov r0, #0x0b      // SIG_SEGV
  b goto_gdb
swi:
  sub lr, lr, #4     // deve retornar para a instrução do trap, não a seguinte
  salva_contexto
  mov r0, #0x05      // SIG_TRAP
  b goto_gdb
irq:
  sub lr, lr, #4
  salva_contexto
  bl trata_irq
  cmp r0, #0
  bne ctrlc
  b switch_back
ctrlc:
  mov r0, #0x02      // SIG_INT
  b goto_gdb

goto_gdb:
  mrs r1, cpsr
  bic r1, #0b11111
  orr r1, #0b10011
  msr cpsr,r1        // modo svc
  b gdb_main

/*
 * Restaura o contexto do programa do usuário e retorna
 */
.global switch_back
switch_back:
   ldr r0, =user_regs
   ldr r1, [r0, #164]
   msr spsr, r1           // spsr do usuário
   ldmib r0, {r1-r14}^    // registradores do usuário
   ldr lr, [r0, #60]      // endereço de retorno
   ldr r0, [r0]           // r0 do usuário
   movs pc, lr            // retorna

/*
 * Suspende o núcleo
 */
trava:
  wfe
  b trava

/*
 * Suspende o processamento por um número de ciclos
 * param r0 Número de ciclos.
 */
.text
.global delay
delay:
  subs r0, r0, #1
  bne delay
  mov pc, lr

/*
 * Habilita ou desabilita interrupções
 * param r0 0 = desabilita, diferente de zero = habilita
 */
.global enable_irq
enable_irq:
  movs r0, r0
  beq disable
  mrs r0, cpsr
  bic r0, r0, #0x80
  msr cpsr_c, r0
  mov pc, lr
disable:
  mrs r0, cpsr
  orr r0, r0, #0x80
  msr cpsr_c, r0
  mov pc, lr

/*
 * Lê o valor atual do CPSR
 */
.globl get_cpsr
get_cpsr:
  mrs r0, cpsr
  mov pc, lr

