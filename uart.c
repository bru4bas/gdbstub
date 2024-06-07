
#include "bcm.h"

#define CTRL_C             0x03

/**
 * Inicia a uart para comunicar 8 bits em 115200 bps
 */
void uart_init(void) {
   uint32_t sel = GPIO_REG(gpfsel[1]);
   sel = (sel & (~(7<<12))) | (2<<12);
   sel = (sel & (~(7<<15))) | (2<<15);
   GPIO_REG(gpfsel[1]) = sel;

   GPIO_REG(gppud) = 0;
   delay(150);
   GPIO_REG(gppudclk[0]) = (1 << 14) | (1 << 15);
   delay(150);
   GPIO_REG(gppudclk[0]) = 0;

   AUX_REG(enables) = 1;
   MU_REG(cntl) = 0;
   MU_REG(ier) = 0;
   MU_REG(lcr) = 3;           // 8 bits
   MU_REG(mcr) = 0;
   MU_REG(baud) = 270;        // para 115200 bps em 250 MHz
   MU_REG(cntl) = 3;          // habilita TX e RX
}

/**
 * Envia um caractere pela uart
 */
void uart_putc(uint8_t c) {
   while((MU_REG(lsr) & 0x20) == 0) ;
   MU_REG(io) = c;
}

/**
 * Envia um string pela uart
 */
void uart_puts(char *s) {
   while(*s) {
      uart_putc(*s);
      s++;
   }
}

/**
 * Recebe um caractere pela uart
 */
uint8_t uart_getc(void) {
   while((MU_REG(lsr) & 0x01) == 0) ;
   return MU_REG(io);
}

/**
 * Habilita interrupção da uart (para identificar ^C).
 */
void uart_break_enable(void) {
   set_bit(MU_REG(ier), 0);
   IRQ_REG(enable_1) = __bit(29);
   enable_irq(1);
}

/**
 * Desabilita interrupção da uart.
 */
void uart_break_disable(void) {
   clr_bit(MU_REG(ier), 0);
   IRQ_REG(disable_1) = __bit(29);
   enable_irq(0);
}

/**
 * Processa interrupção da UART se ativo.
 * Verifica se o caractere ^C (break) foi recebido.
 */
uint32_t trata_irq(void) {
   static uint8_t c = 0;
   static int r;
   r = IRQ_REG(pending_1);
   if(bit_is_set(r, 29)) {                   // interrupção do periférico AUX
      r = AUX_REG(irq);
      if(bit_is_set(r, 0)) {                 // interrupção da UART
         r = (MU_REG(iir) >> 1) & 0x03;     
         if(r == 2) {                        // interrupção de recepção
            c = MU_REG(io);                  // lê o byte recebido
         }
      }
   }
   if(c == CTRL_C) return 1;
   return 0;
}

