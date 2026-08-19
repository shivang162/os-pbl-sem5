# StudyOS Development Setup (Phase 2)

This guide sets up the StudyOS toolchain on **Linux** and **Windows** for boot/kernel development and QEMU testing.

---

## Required Tools

- Git
- VS Code
- GCC cross-compiler (`i686-elf-gcc`)
- GNU Binutils (`i686-elf-ld`, `i686-elf-as`)
- NASM
- GRUB tools (`grub-mkrescue`)
- Make
- QEMU

---

## Linux Setup (Ubuntu/Debian)

## 1) Install base packages

```bash
sudo apt update
sudo apt install -y git make nasm qemu-system-x86 grub-pc-bin xorriso mtools build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

## 2) Install VS Code

Use official package from Microsoft, or:

```bash
sudo snap install code --classic
```

## 3) Build i686-elf cross-compiler

```bash
mkdir -p $HOME/opt/cross $HOME/src
cd $HOME/src

wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar -xf binutils-2.42.tar.xz
mkdir -p build-binutils && cd build-binutils
../binutils-2.42/configure --target=i686-elf --prefix=$HOME/opt/cross --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

wget https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz
tar -xf gcc-14.2.0.tar.xz
mkdir -p build-gcc && cd build-gcc
../gcc-14.2.0/configure --target=i686-elf --prefix=$HOME/opt/cross --disable-nls --enable-languages=c --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc
```

Add to shell profile:

```bash
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

---

## Windows Setup

Recommended: use **WSL2 (Ubuntu)** and follow the Linux setup above.

## 1) Install prerequisites

- Install Git for Windows
- Install VS Code
- Install WSL2 + Ubuntu
- Install QEMU for Windows (or run QEMU from WSL if configured)

PowerShell (Admin):

```powershell
wsl --install
```

Then open Ubuntu terminal and run the Linux setup section.

> Why WSL2? It provides a Linux build environment, which is the simplest path for OS-dev toolchains.

---

## Verification Commands

Run these in Linux/WSL terminal:

```bash
git --version
code --version
i686-elf-gcc --version
i686-elf-ld --version
nasm -v
grub-mkrescue --version
make --version
qemu-system-i386 --version
```

Expected result: each command prints a version and no “command not found” error.

---

## Common Issues

## `i686-elf-gcc: command not found`
- Confirm cross-compiler install path exists: `ls $HOME/opt/cross/bin`
- Re-load PATH: `source ~/.bashrc`

## `grub-mkrescue: command not found`
- Install GRUB packages: `sudo apt install grub-pc-bin xorriso`

## QEMU not launching
- Verify package: `qemu-system-i386 --version`
- On Windows, ensure virtualization is enabled in BIOS and WSL2 is active.

---

## Next Phase

After setup is verified, move to **Phase 3 (Project Structure)**.
