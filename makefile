BUILD_DIR := build
SRC_DIR := src

CFLAGS := -m32
CFLAGS += -fno-builtin
CFLAGS += -nostdinc
CFLAGS += -fno-pic
CFLAGS += -fno-pie
CFLAGS += -nostdlib
CFLAGS += -fno-stack-protector
CFLAGS += -march=pentium
CFLAGS := $(strip ${CFLAGS})

INCLUDE := $(SRC_DIR)/include

KERNEL_ENTRY := 0xc0010000

$(BUILD_DIR)/boot/%.bin : $(SRC_DIR)/boot/%.asm
	mkdir -p $(dir $@)
	nasm -o $@ $^

$(BUILD_DIR)/kernel/%.o : $(SRC_DIR)/kernel/%.asm
	mkdir -p $(dir $@)
	nasm -g -f elf32 -o $@ $^

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	gcc -I$(INCLUDE) $(CFLAGS) -g -c -o $@ $^

$(BUILD_DIR)/kernel.bin : $(BUILD_DIR)/kernel/init.o \
						  $(BUILD_DIR)/kernel/main.o \
						  $(BUILD_DIR)/kernel/io.o \
						  $(BUILD_DIR)/kernel/console.o \
						  $(BUILD_DIR)/lib/string.o \
						  $(BUILD_DIR)/lib/vsprintf.o \
						  $(BUILD_DIR)/kernel/printk.o \
						  $(BUILD_DIR)/kernel/assert.o \
						  $(BUILD_DIR)/kernel/debug.o \
						  $(BUILD_DIR)/kernel/gdt.o \
						  $(BUILD_DIR)/kernel/schedule.o \
						  $(BUILD_DIR)/kernel/intr_handler.o \
						  $(BUILD_DIR)/kernel/intr.o \
						  $(BUILD_DIR)/lib/stdlib.o \
						  $(BUILD_DIR)/kernel/clock.o \
						  $(BUILD_DIR)/kernel/speaker.o \
						  $(BUILD_DIR)/kernel/time.o \
						  $(BUILD_DIR)/kernel/rtc.o \
						  $(BUILD_DIR)/kernel/memory.o \
						  $(BUILD_DIR)/lib/bitmap.o \
						  $(BUILD_DIR)/kernel/thread.o \
						  $(BUILD_DIR)/lib/list.o \
						  $(BUILD_DIR)/kernel/syscall.o \
						  $(BUILD_DIR)/kernel/lock.o \
						  $(BUILD_DIR)/kernel/keyboard.o \
						  $(BUILD_DIR)/lib/fifo.o \
						  $(BUILD_DIR)/kernel/tss.o \
						  $(BUILD_DIR)/kernel/process.o \
						  $(BUILD_DIR)/kernel/ide.o \
						  $(BUILD_DIR)/kernel/device.o \
						  $(BUILD_DIR)/kernel/buffer.o \
						  $(BUILD_DIR)/kernel/fork.o \
						  $(BUILD_DIR)/fs/super.o \
						  $(BUILD_DIR)/fs/bmap.o \
						  $(BUILD_DIR)/fs/inode.o \
						  $(BUILD_DIR)/kernel/system.o \
						  $(BUILD_DIR)/fs/namei.o \
						  $(BUILD_DIR)/fs/file.o \
						  $(BUILD_DIR)/lib/printf.o \
						  $(BUILD_DIR)/builtin/ksh.o
	
	ld -m elf_i386 -static -Ttext $(KERNEL_ENTRY) -o $@ $^


$(BUILD_DIR)/system.bin : $(BUILD_DIR)/kernel.bin
	objcopy -O binary $< $@

$(BUILD_DIR)/system.map : $(BUILD_DIR)/kernel.bin
	nm $< | sort > $@


c.img : $(BUILD_DIR)/boot/mbr.bin \
		$(BUILD_DIR)/boot/loader.bin \
		$(BUILD_DIR)/system.bin \
		$(BUILD_DIR)/system.map \
		$(SRC_DIR)/utils/c.sfdisk
	rm -rf *.img
	bximage -q -func=create -hd=16 -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD_DIR)/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc
	sfdisk $@ < $(SRC_DIR)/utils/c.sfdisk
	chmod +r c.img
	sudo losetup /dev/loop16 --partscan $@
	sudo mkfs.minix -1 -n 14 /dev/loop16p1
	sudo mount /dev/loop16p1 /mnt
	sudo chown ${USER} /mnt
	mkdir -p /mnt/empty
	mkdir -p /mnt/home
	mkdir -p /mnt/d1/d2/d3/d4
	echo "hello kanaos, from root dir file..." > /mnt/hello.txt
	echo "hello kanaos, from home dir file..." > /mnt/home/hello.txt
	sudo umount /mnt
	sudo losetup -d /dev/loop16
	
slave.img :
	bximage -q -func=create -hd=32 -sectsize=512 -imgmode=flat $@
	sfdisk $@ < $(SRC_DIR)/utils/slave.sfdisk
	chmod +r slave.img
	sudo losetup /dev/loop16 --partscan $@
	sudo mkfs.minix -1 -n 14 /dev/loop16p1
	sudo mount /dev/loop16p1 /mnt
	sudo chown ${USER} /mnt
	echo "slave root dir file..." > /mnt/hello.txt
	sudo umount /mnt
	sudo losetup -d /dev/loop16


bochs : c.img slave.img
	rm -rf c.img.lock
	rm -rf *.ini
	bochs -q -f ./bochsrc.txt

bochsg : c.img slave.img
	rm -rf c.img.lock
	rm -rf *.ini
	bochsg -q -f ./bochsgrc.txt

qemu : c.img slave.img
	qemu-system-i386 \
	-m 512M \
	-boot c \
	-drive file=c.img,if=ide,index=0,media=disk,format=raw \
	-drive file=slave.img,if=ide,index=1,media=disk,format=raw \
	-audiodev pa,id=hda \
	-machine pcspk-audiodev=hda \
	-rtc base=localtime

qemug : c.img slave.img
	qemu-system-i386 \
	-s -S \
	-m 512M \
	-boot c \
	-drive file=c.img,if=ide,index=0,media=disk,format=raw \
	-drive file=slave.img,if=ide,index=1,media=disk,format=raw \
	-audiodev pa,id=hda \
	-machine pcspk-audiodev=hda \
	-rtc base=localtime

.PHONY: mount0
mount0: c.img
	sudo losetup /dev/loop16 --partscan $<
	sudo mount /dev/loop16p1 /mnt
	sudo chown ${USER} /mnt 

.PHONY: umount0
umount0: /dev/loop16
	-sudo umount /mnt
	-sudo losetup -d $<

.PHONY: mount1
mount1: slave.img
	sudo losetup /dev/loop16 --partscan $<
	sudo mount /dev/loop16p1 /mnt
	sudo chown ${USER} /mnt 

.PHONY: umount1
umount1: /dev/loop16
	-sudo umount /mnt
	-sudo losetup -d $<

image : c.img slave.img

clean :
	rm -rf $(BUILD_DIR)/*
	rm -rf *.img*
	rm -rf *.ini