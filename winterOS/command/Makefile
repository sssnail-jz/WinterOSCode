# commands/Makefile

#ENTRYPOINT	= 0x1000
HD		= ../../80m.img

ASM		= nasm
DASM	= objdump
CC		= gcc
LD		= ld
ASMFLAGS	= -I ../include/ -f elf
CFLAGS		= -I ../include/ -I ../include/sys/ -c -fno-builtin -fno-stack-protector -Wall
LDFLAGS		= -Ttext 0x1000
DASMFLAGS	= -D
LIB		= ../lib/winterOS.a
BIN		= echo edit pwd clear touch ls rm cat Diary stuSystem game

# All Phony Targets
.PHONY : everything final clean realclean disasm all install

# Default starting position
everything : $(BIN)

install : all realclean
	tar vcf inst.tar $(BIN) hello result.txt
	dd if=inst.tar of=$(HD) seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/load.inc | sed -e 's/.*0x//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/sys/config.h | sed -e 's/.*0x//g'\`)*200" | bc` bs=1 count=`ls -l inst.tar | awk -F " " '{print $$5}'` conv=notrunc

all : realclean everything

final : all clean

clean :
	rm -f *.o

realclean :
	rm -f $(BIN) *.o inst.tar

start.o : start.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

echo.o: echo.c
	$(CC) $(CFLAGS) -o $@ $<

echo : echo.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

pwd.o: pwd.c
	$(CC) $(CFLAGS) -o $@ $<

pwd : pwd.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

ls.o: ls.c
	$(CC) $(CFLAGS) -o $@ $<

ls : ls.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

clear.o: clear.c
	$(CC) $(CFLAGS) -o $@ $<

clear : clear.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

touch.o: touch.c
	$(CC) $(CFLAGS) -o $@ $<

touch : touch.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

rm.o: rm.c
	$(CC) $(CFLAGS) -o $@ $<

rm : rm.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

cat.o: cat.c
	$(CC) $(CFLAGS) -o $@ $<

cat : cat.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

edit.o: edit.c
	$(CC) $(CFLAGS) -o $@ $<

edit : edit.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

Diary.o: Diary.c
	$(CC) $(CFLAGS) -o $@ $<

Diary : Diary.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

stuSystem.o: stuSystem.c
	$(CC) $(CFLAGS) -o $@ $<

stuSystem : stuSystem.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

game.o: game.c
	$(CC) $(CFLAGS) -o $@ $<

game : game.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?
