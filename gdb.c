
/**
 * Implementação trivial do protocolo gdb remote serial
 * Detalhes em https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html
 */

#include "uart.h"
#include <stdbool.h>
void delay(unsigned);

/*
 * Sinais reconhecidos pelo GDB
 */
#define SIG_HUP            0x01
#define SIG_INT            0x02
#define SIG_QUIT           0x03
#define SIG_ILL            0x04
#define SIG_TRAP           0x05
#define SIG_ABRT           0x06
#define SIG_KILL           0x09
#define SIG_SEGV           0x0b
#define SIG_SYS            0x0a
#define SIG_TERM           0x0f
#define SIG_STOP           0x11

/*
 * Instrução usada como trap (breakpoint)
 */
#define TRAP_INST          0xefaaaaaa

#define MEMORY(X)          *((uint32_t*)(X))

/*
 * Símbolos declarados pelo linker
 */
extern uint8_t *stack_svr, *load_addr;

/*
 * Valor dos registradores do usuário.
 */
#define NUM_REGS           42
uint32_t user_regs[NUM_REGS] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // r0-r12
   (uint32_t)&stack_svr,                      // sp
   0,                                         // lr
   (uint32_t)&load_addr,                      // pc
   0, 0, 0, 0, 0, 0, 0, 0,                    // f0-f7 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // no idea
   0,                                         // fps
   0x10                                       // cpsr
};
uint8_t user_status = SIG_TRAP;

#define PC                 (user_regs[15])
#define CPSR               (user_regs[41])

// 0-15  --- r0-r15
// 16-23 --- f0-f7
// 24    --- FPS (floating point status) na escrita ?
// 25    --- processor status na escrita ?   FIXME: copiar status em algum momento?
// 26-39 --- não tenho a menor ideia
// 40    --- fps
// 41    --- cpsr

/*
 * Breakpoints.
 */
#define MAX_BKPTS          16
typedef struct bkpt_s {
   uint32_t addr;
   uint32_t cont;
} bkpt_t;
bkpt_t bkpts[MAX_BKPTS] = { 0 };

/*
 * Funções para manipulação de caracteres hexadecimais.
 */
char hex_to_char(int n) {
   if(n < 10) return '0' + n;
   if(n < 16) return 'a' + n - 10;
   return '0';
}

int char_to_hex(char c) {
   if((c >= '0') && (c <= '9')) return c - '0';
   if((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
   if((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
   return 0;
}

/**
 * Envia um byte em hexadecimal pela uart.
 * @param v Valor a enviar (8 bits).
 * @return Checksum dos caracteres enviados.
 */
uint8_t sendbyte(uint8_t v) {
   uint8_t chk = 0, c;
   c = hex_to_char(v >> 4);
   uart_putc(c);  chk += c;
   c = hex_to_char(v & 0x0f);
   uart_putc(c);  chk += c;
   return chk;
}

/**
 * Recebe um byte (dois caracteres hexadecimais) pela uart.
 * @return Byte recebido.
 */
uint8_t readbyte(void) {
   int res = char_to_hex(uart_getc());
   return (res << 4) | char_to_hex(uart_getc());
}

/**
 * Envia uma mensagem completa contendo os dados de uma área de memória.
 * @param a Endereço da área de memória.
 * @param s Quantidade de bytes a enviar.
 */
void sendbytes(uint8_t *a, uint32_t s) {
   uint8_t chk = 0;
   uint8_t c;

   uart_putc('$');
   while(s) {
      chk += sendbyte(*a);
      a++;
      s--;
   }
   uart_putc('#');
   sendbyte(chk);
}

/**
 * Recebe uma quantidade de bytes (caracteres hexadecimais) pela uart.
 * @param a Endereço inicial para salvar os dados recebidos.
 * @param s Quantidade de bytes a receber.
 */
void readbytes(uint8_t *a, uint32_t s) {
   while(s) {
      *a++ = readbyte();
      s--;
   }
}

/**
 * Recebe um inteiro de 32 bits (de um a oito caracteres hexadecimais, 
 * delimitados por um caractere específico).
 * @param t Caractere delimitador.
 * @return Valor uint32_t recebido.
 */
uint32_t readword(char t) {
   uint32_t res = 0;
   uint8_t c;
   for(;;) {
      c = uart_getc();
      if(c == t) return res;
      res = (res << 4) | char_to_hex(c);
   }
}

/**
 * Troca o endianess de big para little ou vice-versa.
 */
uint32_t endian_change(uint32_t v) {
   uint32_t res = 0;
   res |= (v & 0xff) << 24;
   res |= (v & 0xff00) << 8;
   res |= (v & 0xff0000) >> 8;
   res |= (v & 0xff000000) >> 24;
   return res;
}

/**
 * Recebe e descarta caracteres pela uart até a recepção do 
 * caractere especificado.
 * @param t Caractere a receber.
 */
void skip(char t) {
   for(;;) {
      uint8_t c = uart_getc();
      if(c == t) return;
   }
}

/**
 * Aguarda a conclusão de uma mensagem e envia um ack ('+').
 */
void ack(void) {
   skip('#');
   uart_getc();                     // ignora checksum
   uart_getc();
   uart_putc('+');                  // responde com um acknowledge
}

/**
 * Acrescenta um novo breakpoint na lista.
 * @param addr Endereço da memória para o breakpoint.
 * @return false se não houver espaço para um novo breakpoint.
 */
bool bkpt_add(uint32_t addr) {
   int j = 0;
   if(addr == 0) return false;
   for(int i=1; i<MAX_BKPTS; i++) {
      if(bkpts[i].addr == addr) return true;           // breakpoint redefinido
      if(bkpts[i].addr == 0) j = i;                    // posição vaga
   }

   if(j == 0) return false;                            // nenhuma posição vaga
   bkpts[j].addr = addr;
   return true;
}

/**
 * Remove um breakpoint da lista.
 * @param addr Endereço de memória do breakpoint.
 * @return false se o breakpoint não foi encontrado.
 */
bool bkpt_remove(uint32_t addr) {
   for(int i=1; i<MAX_BKPTS; i++) {
      if(bkpts[i].addr == addr) {
         bkpts[i].addr = 0;
         return true;
      }
   }
   return false;
}

/**
 * Limpa a memória de instruções trap.
 */
void bkpt_restore_contents(void) {
   for(int i=0; i<MAX_BKPTS; i++) {
      uint32_t addr = bkpts[i].addr;
      if(addr == 0) continue;
      MEMORY(addr) = bkpts[i].cont;
   }
}

/**
 * Coloca traps na memória nas posições dos breakpoints ativos.
 */
void bkpt_activate(void) {
   for(int i=0; i<MAX_BKPTS; i++) {
      uint32_t addr = bkpts[i].addr;
      if(addr == 0) continue;
      bkpts[i].cont = MEMORY(addr);
      MEMORY(addr) = TRAP_INST;
   }
}

/**
 * Ponto de entrada do loop de processamento de mensagens do stub.
 */
void gdb_main(int sig) {
   static uint32_t a, s;
   static uint8_t chk;
   static uint8_t c;

   /*
    * Limpa brakepoints da memória
    */
   uart_break_disable();
   bkpt_restore_contents();
   bkpts[0].addr = 0;

   /*
    * Envia status ao depurador.
    */
   user_status = sig;
   uart_puts("$S");
   chk = 'S' + sendbyte(user_status);
   uart_putc('#');
   sendbyte(chk);

   /*
    * Espera uma mensagem.
    */
retry:
   skip('$');

   /*
    * Identifica a mensagem
    */
   c = uart_getc();
   switch(c) {
      case '?':
         goto trata_status;
      case 'g':
         goto trata_g;
      case 'G':
         goto trata_G;
      case 'P':
         goto trata_P;
      case 'm':
         goto trata_m;
      case 'M':
         goto trata_M;
      case 'c':
         ack();
         goto executa;
      case 's':
         goto trata_s;
      case 'Z':
         c = uart_getc();
         if(c == '0') goto trata_Z0;
         break;
      case 'z':
         c = uart_getc();
         if(c == '0') goto trata_z0;
         break;
      case 'D':
      case 'k':
         ack();
         goto envia_ok;
   }
   ack();

   /*
    * Comando não reconhecido
    */
envia_nulo:
   uart_puts("$#00");
   goto retry;

envia_ok:
   uart_puts("$OK#9a");
   goto retry;

envia_erro:
   uart_puts("$E01#a5");
   goto retry;

executa:
   bkpt_activate();
   uart_break_enable();
   asm volatile ("b switch_back");
   goto retry;

trata_g:
   /*
    * Envia todos os registradores.
    */
   ack();
   sendbytes((uint8_t*)user_regs, sizeof(user_regs));
   goto retry;

trata_G:
   /*
    * Altera todos os registradores.
    */
   readbytes((uint8_t*)user_regs, sizeof(user_regs));
   ack();
   goto envia_ok;

trata_P:
   /*
    * Altera um dos registradores.
    */
   a = readword('=');                   // índice do registrador
   s = readword('#');
   uart_getc();
   uart_getc();
   uart_putc('+');
   if(a <= NUM_REGS) {
      user_regs[a] = endian_change(s);
   }
   goto envia_ok;

trata_m:
   /*
    * Lê memória.
    */
   a = readword(',');                   // endereço inicial
   s = readword('#');                   // tamanho
   uart_getc();
   uart_getc();
   uart_putc('+');

   sendbytes((uint8_t*)a, s);
   goto retry;

trata_M:
   /*
    * Escreve memória.
    */
   a = readword(',');                   // endereço inicial
   s = readword(':');                   // tamanho

   readbytes((uint8_t*)a, s);
   ack();
   goto envia_ok;

trata_status:
   /*
    * Envia o último sinal.
    */
   ack();
   uart_puts("$S");
   chk = 'S' + sendbyte(user_status);
   uart_putc('#');
   sendbyte(chk);
   goto retry;

trata_s:
   /*
    * Executa a próxima instrução.
    * Introduz um trap na instrução seguinte.
    * (não funciona se for um salto...)
    */
   ack();
   bkpts[0].addr = PC + 4;
   goto executa;

trata_Z0:
   /*
    * Inclui um breakpoint de software.
    */
   uart_getc();        // ','
   a = readword(',');
   ack();
   if(bkpt_add(a)) goto envia_ok;
   goto envia_erro;

trata_z0:
   /*
    * Remove um breakpoint de software.
    */
   uart_getc();        // ','
   a = readword(',');
   ack();
   bkpt_remove(a);
   goto envia_ok;
}

/**
 * Inicialização em C.
 */
void main(void) {
   uart_init();

   delay(100);
   uart_puts("Kernel Panic gdb stub\r\n");
   uart_puts("The risk is all yours\r\n");
   asm volatile (
      "mov r0, #0x05 \n\t"
      "b gdb_main \n\t"
   );
}

