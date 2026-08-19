# StudyOS — AI-Powered Student Mini Operating System

StudyOS is a beginner-friendly educational operating system project focused on student productivity.

## Current Phase

Phase 1 (Architecture) and Phase 2 (Development Setup Guide) are initialized in this repository.

## Documentation

- `/home/runner/work/os-pbl-sem5/os-pbl-sem5/docs/architecture.md`
- `/home/runner/work/os-pbl-sem5/os-pbl-sem5/docs/setup.md`

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