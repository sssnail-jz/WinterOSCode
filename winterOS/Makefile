ENTRYPOINT	= 0x1000

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -I include/sys/ -f elf
CFLAGS		= -I include/ -I include/sys/ -c -fno-builtin -Wall
#CFLAGS		= -I include/ -c -fno-builtin -fno-stack-protector -fpack-struct -Wall
LDFLAGS		= -Ttext $(ENTRYPOINT)
ARFLAGS		= rcs

# This Program
WINTEROSBOOT	= boot/boot.bin boot/loader.bin
WINTEROSKERNEL	= kernel.bin
LIB		= lib/winterOS.a

OBJS	= 	kernel/kernel.o kernel/start.o kernel/main.o kernel/clock.o \
			kernel/i8259.o kernel/global.o kernel/protect.o kernel/systask.o \
			kernel/klib/kliba.o kernel/klib/klib.o kernel/klib/dbg_tool.o \
			fs/main.o fs/do_open.o fs/misc.o fs/do_rdwt.o fs/do_link.o \
			hd/hd.o \
			ipc/proc.o\
			mm/main.o mm/do_forkexit.o mm/do_exec.o\
			tty/keyboard.o tty/tty.o tty/console.o

LOBJS	=	lib/fs/close.o lib/fs/open.o lib/fs/read.o lib/fs/stat.o lib/fs/seek.o lib/fs/unlink.o \
			lib/fs/write.o lib/fs/files_info.o lib/fs/clear.o\
			lib/mm/exec.o lib/mm/exit.o lib/mm/fork.o lib/mm/wait.o \
			lib/sys/get_pid.o lib/sys/get_ticks.o lib/sys/get_time.o lib/sys/set_color.o \
			lib/tools/printf.o lib/tools/scanf.o lib/tools/misc.o lib/tools/vsprintf.o \
			lib/tools/string.o lib/tools/syscall.o lib/tools/getch.o

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
nop :
	@echo "why not \`make image' huh? :)"

everything : $(WINTEROSBOOT) $(WINTEROSKERNEL)

all : realclean everything

image : realclean everything clean buildimg

clean :
	rm -f $(OBJS) $(LOBJS)

realclean :
	rm -f $(OBJS) $(LOBJS) $(WINTEROSBOOT) $(WINTEROSKERNEL)

# We assume that "winterOS.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=winterOS.img bs=512 count=1 conv=notrunc
	sudo mount -o loop winterOS.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(WINTEROSKERNEL) : $(OBJS) $(LIB)
	$(LD) $(LDFLAGS) -o $(WINTEROSKERNEL) $^

$(LIB) : $(LOBJS)
	$(AR) $(ARFLAGS) $@ $^

tty/keyboard.o: tty/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<

tty/tty.o: tty/tty.c
	$(CC) $(CFLAGS) -o $@ $<

tty/console.o: tty/console.c
	$(CC) $(CFLAGS) -o $@ $<

ipc/proc.o: ipc/proc.c
	$(CC) $(CFLAGS) -o $@ $<
	
hd/hd.o: hd/hd.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/kernel.o : kernel/kernel.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o: kernel/i8259.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/systask.o: kernel/systask.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/klib/klib.o: kernel/klib/klib.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/klib/kliba.o : kernel/klib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/klib/dbg_tool.o: kernel/klib/dbg_tool.c
	$(CC) $(CFLAGS) -o $@ $<

mm/do_forkexit.o: mm/do_forkexit.c
	$(CC) $(CFLAGS) -o $@ $<

mm/do_exec.o: mm/do_exec.c
	$(CC) $(CFLAGS) -o $@ $<

mm/main.o: mm/main.c
	$(CC) $(CFLAGS) -o $@ $<

fs/main.o: fs/main.c
	$(CC) $(CFLAGS) -o $@ $<

fs/do_open.o: fs/do_open.c
	$(CC) $(CFLAGS) -o $@ $<

fs/do_rdwt.o: fs/do_rdwt.c
	$(CC) $(CFLAGS) -o $@ $<

fs/do_link.o: fs/do_link.c
	$(CC) $(CFLAGS) -o $@ $<

fs/misc.o: fs/misc.c
	$(CC) $(CFLAGS) -o $@ $<

# lib begin ...
lib/fs/close.o: lib/fs/close.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/open.o: lib/fs/open.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/read.o: lib/fs/read.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/stat.o: lib/fs/stat.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/seek.o: lib/fs/seek.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/unlink.o: lib/fs/unlink.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/write.o: lib/fs/write.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/printf.o: lib/tools/printf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/scanf.o: lib/tools/scanf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/files_info.o: lib/fs/files_info.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/clear.o: lib/fs/clear.c
	$(CC) $(CFLAGS) -o $@ $<

lib/mm/exec.o: lib/mm/exec.c
	$(CC) $(CFLAGS) -o $@ $<

lib/mm/exit.o: lib/mm/exit.c
	$(CC) $(CFLAGS) -o $@ $<

lib/mm/fork.o: lib/mm/fork.c
	$(CC) $(CFLAGS) -o $@ $<

lib/mm/wait.o: lib/mm/wait.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/get_pid.o: lib/sys/get_pid.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/get_ticks.o: lib/sys/get_ticks.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/get_time.o: lib/sys/get_time.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/set_color.o: lib/sys/set_color.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/misc.o: lib/tools/misc.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/vsprintf.o: lib/tools/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/getch.o: lib/tools/getch.c
	$(CC) $(CFLAGS) -o $@ $<

lib/tools/string.o: lib/tools/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/tools/syscall.o: lib/tools/syscall.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
