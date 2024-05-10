# GNUmakefile: Main makefile of the project.
# Code is governed by the GPL-2.0 license.
# Copyright (C) 2021-2022 The Vinix authors.

QEMUFLAGS ?= -M q35,smm=off -m 1G -cdrom nyaux.iso -serial stdio -smp 4

.PHONY: all
all:
	rm -f nyaux.iso
	$(MAKE) nyaux.iso

nyaux.iso: jinx
	rm -f builds/kernel.built builds/kernel.packaged
	$(MAKE) distro-base
	./build-support/makeiso.sh

.PHONY: debug
debug:
	JINX_CONFIG_FILE=jinx-config $(MAKE) all

jinx:
	curl -Lo jinx https://github.com/mintsuki/jinx/raw/802082d0389d0b73afed5f52875a204e9134a3fe/jinx
	chmod +x jinx

.PHONY: distro-full
distro-full: jinx
	./jinx build-all

.PHONY: distro-base
distro-base: jinx
	./jinx build base-files kernel

.PHONY: run-kvm
run-kvm: nyaux.iso
	qemu-system-x86_64 -enable-kvm -cpu host $(QEMUFLAGS)

.PHONY: run-hvf
run-hvf: nyaux.iso
	qemu-system-x86_64 -accel hvf -cpu host $(QEMUFLAGS)

ovmf:
	mkdir -p ovmf
	cd ovmf && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd

.PHONY: run-uefi
run-uefi: nyaux.iso ovmf
	qemu-system-x86_64 $(QEMUFLAGS) -cpu max -bios ovmf/OVMF.fd -d int

.PHONY: run-bochs
run-bochs: nyaux.iso
	bochs -f bochsrc

.PHONY: run-lingemu
run-lingemu: nyaux.iso
	lingemu runvirt -m 8192 --diskcontroller type=ahci,name=ahcibus1 --disk nyaux.iso,disktype=cdrom,controller=ahcibus1

.PHONY: run
run: nyaux.iso
	qemu-system-x86_64 $(QEMUFLAGS)
.PHONY: run-debug
run-debug: nyaux.iso
	qemu-system-x86_64 $(QEMUFLAGS) -s -S -cpu max

.PHONY: kernel-clean
kernel-clean:
	make -C kernel clean
	rm -rf builds/kernel* pkgs/kernel*



.PHONY: init-clean
init-clean:
	rm -rf init/init
	rm -rf builds/init* pkgs/init*

.PHONY: base-files-clean
base-files-clean:
	rm -rf builds/base-files* pkgs/base-files*

.PHONY: clean
clean: kernel-clean init-clean base-files-clean
	rm -rf iso_root sysroot nyaux.iso initramfs.tar

.PHONY: distclean
distclean: jinx
	make -C kernel distclean
	./jinx clean
	rm -rf iso_root sysroot nyaux.iso initramfs.tar jinx ovmf
	chmod -R 777 .jinx-cache
	rm -rf .jinx-cache