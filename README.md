# StudyOS — AI-Powered Student Mini Operating System

StudyOS is a beginner-friendly educational operating system project focused on student productivity.

## Current Phase

Phase 1 (Architecture), Phase 2 (Development Setup Guide), Phase 3 (Project Structure), and Phase 4 (GRUB Multiboot boot path + bootable ISO milestone) are completed.
Phase 5 (Basic Kernel Terminal) is now started with core terminal output behavior improvements.

## Documentation

- `/home/runner/work/os-pbl-sem5/os-pbl-sem5/docs/architecture.md`
- `/home/runner/work/os-pbl-sem5/os-pbl-sem5/docs/setup.md`

## Phase 3 Structure Status

Initial source skeletons are now added for:
- boot entry
- kernel core + terminal + memory
- drivers (keyboard/screen)
- filesystem
- security
- shell
- top-level `Makefile` and `linker.ld`

## High-Level Architecture

- **Boot layer**: GRUB + entry assembly
- **Kernel layer**: memory/process basics, terminal I/O, keyboard input, syscall interface
- **User-space services**: productivity, gamification, smart behavior, voice command adapter, cloud simulation
- **Applications/UI**: shell commands, dashboard, notes, tasks, timetable, editor tools

## Development Roadmap

Architecture → Setup → Structure → Boot → Basic Kernel → Shell → Filesystem → Security → Productivity → Gamification → Smart Behavior → Voice → Developer Tools → StudyCloud → UI Polish → Testing → Final Documentation

## First Milestone

Boot a StudyOS ISO in QEMU and display:

`StudyOS Kernel Started Successfully!`

## Build and Run (Phase 4)

```bash
make all
make iso
make run
```