
FONTES = gdb.c uart.c boot.s  

#
# Arquivos de saída 
#
EXEC = kernel.elf
MAP = kernel.map
IMAGE = kernel.img
HEXFILE = kernel.hex
LIST = kernel.list

PREFIXO = arm-none-eabi-
LDSCRIPT = kernel.ld
AS = ${PREFIXO}as
LD = ${PREFIXO}ld
GCC = ${PREFIXO}gcc
OBJCPY = ${PREFIXO}objcopy
OBJDMP = ${PREFIXO}objdump

# Para RPi Zero
#OPTS = -mfpu=vfp -march=armv6zk -mtune=arm1176jzf-s -g 

#Para PPi 2
OPTS = -march=armv7-a -mtune=cortex-a7 -g 

#LDOPTS = -L/usr/lib/arm-none-eabi/newlib -lc -lm
OBJ = $(FONTES:.s=.o)
OBJETOS = $(OBJ:.c=.o)

all: ${EXEC} ${IMAGE} ${LIST} ${HEXFILE}

#
# Gerar executável
#
${EXEC}: ${OBJETOS}
	${LD} -T ${LDSCRIPT} -M=${MAP} -o $@  ${OBJETOS} ${LDOPTS}

#
# Gerar imagem
#
${IMAGE}: ${EXEC}
	${OBJCPY} ${EXEC} -O binary ${IMAGE}

#
# Gerar intel Hex
#
${HEXFILE}: ${EXEC}
	${OBJCPY} ${EXEC} -O ihex ${HEXFILE}

#
# Gerar listagem
#
${LIST}: ${EXEC}
	${OBJDMP} -d ${EXEC} > ${LIST}

#
# Compilar arquivos em C
#
.c.o:
	${GCC} ${OPTS} -c -o $@ $<

#
# Montar arquivos em assembler
#
.s.o:
	${AS} -g -o $@ $<

#
# Limpar tudo
#
clean:
	rm -f *.o ${EXEC} ${MAP} ${LIST} ${IMAGE}

