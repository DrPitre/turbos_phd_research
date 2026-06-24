# Demo for CMOC/Turbo9 port of FreeRTOS
#   Author: Boisy Pitre
#
SRCDIR=../..
RTOS_BASEDIR=$(SRCDIR)/Source
#include ".././.
CPU=turbo9

DEVC_PREFIX=
CC=$(DEVC_PREFIX)cmoc
AS=$(DEVC_PREFIX)as
AR=$(DEVC_PREFIX)ar
\OBJDUMP=$(DEVC_PREFIX)lwobjdump

CPPFLAGS+=-i -I. -I./asm-$(CPU)/arch-turbo9 -I../Common/include -I../../Source/portable/CMOC/Turbo9 \
  -I$(RTOS_BASEDIR)/include -DCMOC_TURBO9

CFLAGS+=-i -D$(CPU) -Wno-const -DFREERTOS

#  ../Common/Minimal/PollQ.c

# vectors.c serial.c sci.c ParTest.c gelfunc.c \
#  ../Common/Minimal/flash.c \
#  ../Common/Minimal/dynamic.c \
#  ../Common/Minimal/BlockQ.c \
#  ../Common/Minimal/PollQ.c \
#  ../Common/Minimal/comtest.c \
#  ../Common/Minimal/integer.c \
#  ../Common/Minimal/death.c \

RTOS_OBJS = main.c startup.c $(RTOS_BASEDIR)/portable/CMOC/Turbo9/port.c \
  $(RTOS_BASEDIR)/portable/MemMang/heap_2.c \
  $(RTOS_BASEDIR)/list.c \
  $(RTOS_BASEDIR)/tasks.c
#  $(RTOS_BASEDIR)/queue.c

RTOS_OBJS_C_EXEMPLARS = main_exemplars.c startup.c $(RTOS_BASEDIR)/portable/CMOC/Turbo9/port.c \
  $(RTOS_BASEDIR)/portable/MemMang/heap_2.c \
  $(RTOS_BASEDIR)/list.c \
  $(RTOS_BASEDIR)/tasks.c \
  ../../../turbos/source/c/rlenc.c \
  ../../../turbos/source/c/rldec.c \
  ../../../turbos/source/c/aclamp.c
#  $(RTOS_BASEDIR)/queue.c

RTOS_OBJS_ASM_EXEMPLARS = main_exemplars.c startup.c $(RTOS_BASEDIR)/portable/CMOC/Turbo9/port.c \
  $(RTOS_BASEDIR)/portable/MemMang/heap_2.c \
  $(RTOS_BASEDIR)/list.c \
  $(RTOS_BASEDIR)/tasks.c \
  ../../../turbos/source/turbo9asm/rlenc_inc.asm \
  ../../../turbos/source/turbo9asm/rldec_inc.asm \
  ../../../turbos/source/turbo9asm/aclamp_inc.asm
#  $(RTOS_BASEDIR)/queue.c

OBJS=$(RTOS_OBJS:.c=.o)
OBJS_C_EXEMPLARS=$(RTOS_OBJS_C_EXEMPLARS:.c=.o)
OBJS_ASM_EXEMPLARS=$(RTOS_OBJS_ASM_EXEMPLARS:.c=.o)

RTOS_OBJS_TEST = main_test.c startup.c $(RTOS_BASEDIR)/portable/CMOC/Turbo9/port.c \
  $(RTOS_BASEDIR)/portable/MemMang/heap_2.c \
  $(RTOS_BASEDIR)/list.c \
  $(RTOS_BASEDIR)/tasks.c

OBJS_TEST=$(RTOS_OBJS_TEST:.c=.o)

all:	freertos_baseline.img freertos_c_exemplars.img freertos_test.img
	@ls -l freertos_baseline.obj
	@ls -l freertos_c_exemplars.obj
	@ls -l freertos_test.obj

freertos_baseline.obj:	$(OBJS)
	$(CC) -i $(LDFLAGS) --raw -o $@ $^ 

freertos_c_exemplars.obj:	$(OBJS_C_EXEMPLARS)
	$(CC) -i $(LDFLAGS) --raw -o $@ $^ 
	
freertos_test.obj:	$(OBJS_TEST)
	$(CC) -i $(LDFLAGS) --raw -o $@ $^

freertos_test.img: freertos_test.obj lastpage
	dd if=/dev/zero of=zero.tmp bs=10240 count=1
	cat zero.tmp freertos_test.obj > $@.tmp
	os9 padrom -c=0 65280 $@.tmp
	cat $@.tmp lastpage > $@

freertos_asm_exemplars.obj:	$(OBJS_ASM_EXEMPLARS)
	$(CC) -i $(LDFLAGS) --raw -o $@ $^ 
	
clean:
	$(RM) $(OBJS) $(OBJS_C_EXEMPLARS) $(OBJS_TEST) *.obj *.img *.rom *.tmp *.list *.map lastpage

freertos_baseline.img: freertos_baseline.obj lastpage
	dd if=/dev/zero of=zero.tmp bs=10240 count=1
	cat zero.tmp freertos_baseline.obj > $@.tmp
	os9 padrom -c=0 65280 $@.tmp
	cat $@.tmp lastpage > $@
	
freertos_c_exemplars.img: freertos_c_exemplars.obj lastpage
	dd if=/dev/zero of=zero.tmp bs=10240 count=1
	cat zero.tmp freertos_c_exemplars.obj > $@.tmp
	os9 padrom -c=0 65280 $@.tmp
	cat $@.tmp lastpage > $@
	
freertos_asm_exemplars.img: freertos_asm_exemplars.obj lastpage
	dd if=/dev/zero of=zero.tmp bs=10240 count=1
	cat zero.tmp freertos_asm_exemplars.obj > $@.tmp
	os9 padrom -c=0 65280 $@.tmp
	cat $@.tmp lastpage > $@
	
lastpage: lastpage.asm
	lwasm lastpage.asm --raw -olastpage

