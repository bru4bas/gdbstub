
#ifndef __BCM_H__
#define __BCM_H__

#include <stdint.h>

/*
 * Macros úteis
 */
#define __bit(X)             (0x01 << X)
#define set_bit(X,Y)         X |= (0x01 << Y)
#define clr_bit(X,Y)         X &= (~(0x01 << Y))
#define bit_is_set(X, Y)     (X & (0x01 << Y))
#define bit_not_set(X, Y)    ((X & (0x01 << Y)) == 0)

/*
 * Endereços dos periféricos
 */
#ifndef RPICPU
#error Versão do Raspberry pi não definida
#endif

#if RPICPU == 2
#define PERIPH_BASE  0x3f000000
#elif RPICPU == 0
#define PERIPH_BASE  0x20000000
#else
#endif

#define GPIO_ADDR    (PERIPH_BASE + 0x200000)
#define AUX_ADDR     (PERIPH_BASE + 0x215000)
#define AUX_MU_ADDR  (PERIPH_BASE + 0x215040)
#define TIMER_ADDR   (PERIPH_BASE + 0x00B400)
#define IRQ_ADDR     (PERIPH_BASE + 0x00B200)
#define DMA_BASE     (PERIPH_BASE + 0x7000)
#define DMA0_ADDR    (DMA_BASE + 0)
#define DMA1_ADDR    (DMA_BASE + 0x100)
#define DMA2_ADDR    (DMA_BASE + 0x200)
#define DMA3_ADDR    (DMA_BASE + 0x300)
#define DMA4_ADDR    (DMA_BASE + 0x400)
#define DMA5_ADDR    (DMA_BASE + 0x500)
#define DMA6_ADDR    (DMA_BASE + 0x600)
#define DMA7_ADDR    (DMA_BASE + 0x700)
#define DMA8_ADDR    (DMA_BASE + 0x800)
#define DMA9_ADDR    (DMA_BASE + 0x900)
#define DMA10_ADDR   (DMA_BASE + 0xa00)
#define DMA11_ADDR   (DMA_BASE + 0xb00)
#define DMA12_ADDR   (DMA_BASE + 0xc00)
#define DMA13_ADDR   (DMA_BASE + 0xd00)
#define DMA14_ADDR   (DMA_BASE + 0xe00)
#define DMA_STATUS_ADDR (DMA_BASE + 0xfe0)
#define DMA_ENABLE_ADDR (DMA_BASE + 0xff0)

/*
 * Periférico GPIO
 */
typedef struct {
   uint32_t gpfsel[6];   // Function select (3 bits/gpio)
   unsigned : 32;
   uint32_t gpset[2];    // Output set (1 bit/gpio)
   unsigned : 32;
   uint32_t gpclr[2];    // Output clear (1 bit/gpio)
   unsigned : 32;
   uint32_t gplev[2];    // Input read (1 bit/gpio)
   unsigned : 32;
   uint32_t gpeds[2];    // Event detect status
   unsigned : 32;
   uint32_t gpren[2];    // Rising-edge detect enable
   unsigned : 32;
   uint32_t gpfen[2];    // Falling-edge detect enable
   unsigned : 32;
   uint32_t gphen[2];    // High level detect enable
   unsigned : 32;
   uint32_t gplen[2];    // Low level detect enable
   unsigned : 32;
   uint32_t gparen[2];   // Async rising-edge detect
   unsigned : 32;
   uint32_t gpafen[2];   // Async falling-edge detect
   unsigned : 32;
   uint32_t gppud;       // Pull-up/down enable
   uint32_t gppudclk[2]; // Pull-up/down clock enable
} gpio_reg_t;
#define GPIO_REG(X)  ((gpio_reg_t*)(GPIO_ADDR))->X

/*
 * Periférico AUX
 */
typedef struct {
   uint32_t irq;
   uint32_t enables;
} aux_reg_t;
#define AUX_REG(X)   ((aux_reg_t*)(AUX_ADDR))->X

/*
 * AUX/Mini UART
 */
typedef struct {
   uint32_t io;
   uint32_t ier;
   uint32_t iir;
   uint32_t lcr;
   uint32_t mcr;
   uint32_t lsr;
   uint32_t msr;
   uint32_t scratch;
   uint32_t cntl;
   uint32_t stat;
   uint32_t baud;
} mu_reg_t;
#define MU_REG(X)    ((mu_reg_t*)(AUX_MU_ADDR))->X

/*
 * Timer
 */
typedef struct {
   uint32_t load;
   uint32_t value;
   uint32_t control;
   uint32_t ack;
   uint32_t raw_irq;
   uint32_t masked_irq;
   uint32_t reload;
   uint32_t pre;
   uint32_t counter;
} timer_reg_t;
#define TIMER_REG(X)   ((timer_reg_t*)(TIMER_ADDR))->X

/*
 * Controlador de interrupções.
 */
typedef struct {
   uint32_t pending_basic;
   uint32_t pending_1;
   uint32_t pending_2;
   uint32_t fiq;
   uint32_t enable_1;
   uint32_t enable_2;
   uint32_t enable_basic;
   uint32_t disable_1;
   uint32_t disable_2;
   uint32_t disable_basic;
} irq_reg_t;
#define IRQ_REG(X)     ((irq_reg_t*)(IRQ_ADDR))->X

/*
 * Controlador de DMA
 */
typedef struct {
   uint32_t ti;
   uint32_t saddr;
   uint32_t daddr;
   uint32_t length;
   uint32_t stride;
   uint32_t nextcb;
   unsigned : 32;
   unsigned : 32;
} dma_cb_t;

typedef struct {
   uint32_t cs;
   uint32_t cb;
   uint32_t ti;
   uint32_t saddr;
   uint32_t daddr;
   uint32_t length;
   uint32_t stride;
   uint32_t nextcb;
   uint32_t debug;
} dma_reg_t;
#define DMA_CHN_REG(X,Y)  ((dma_reg_t*)(DMA ## X ## _ADDR))->Y
#define DMA_STATUS_REG (*(uint32_t*)DMA_STATUS_ADDR)
#define DMA_ENABLE_REG (*(uint32_t*)DMA_ENABLE_ADDR)

/*
 * Funções em assembler
 */
void delay(uint32_t dur);
uint32_t get_cpsr(void);
void enable_irq(uint32_t en);

#endif
