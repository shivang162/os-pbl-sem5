# StudyOS Architecture (Phase 1)

## 1) What is an Operating System?

An Operating System (OS) is the main software layer between hardware and user applications.  
It manages CPU time, memory, storage, input/output devices, and provides common services to programs.

## 2) What does the Kernel do?

The kernel is the core of the OS. In StudyOS, it is responsible for:

- Process/task control
- Memory management basics
- Device access (keyboard/screen/disk interface)
- System calls (interface between apps/services and kernel)
- Security boundaries and permission checks

## 3) What does a Bootloader do?

A bootloader starts after BIOS/UEFI firmware.  
It loads the kernel into memory and jumps to the kernel entry point.

StudyOS will use **GRUB Multiboot** so we can focus on kernel development instead of writing a full bootloader from scratch.

## 4) How StudyOS boots

```text
BIOS/UEFI
   ↓
GRUB (Multiboot)
   ↓
Kernel entry point
   ↓
Kernel initialization
   - screen
   - memory basics
   - keyboard/input
   - filesystem interface stubs
   - service startup hooks
   ↓
StudyOS shell/dashboard prompt
```

## 5) Kernel Space vs User Space

### Kernel Space
- Privileged, low-level code
- Hardware control
- Core system management

### User Space
- Safer application/service layer
- Features that should not make the kernel complex:
  - tasks/notes/timetable
  - smart recommendations
  - voice command adapter
  - cloud-like synchronization simulation

## 6) Where major StudyOS features fit

### Kernel
- Boot and initialization
- Terminal output/input foundation
- System call interface
- Storage and permission enforcement hooks

### User-space services and apps
- Smart Behavior Engine (rule-based)
- Student Productivity (tasks, notes, timetable, dashboard)
- Gamification (XP, levels, streaks, achievements)
- Voice Command Service (speech result → command mapping)
- Developer tools (simple editor/compile/run flow)
- StudyCloud simulation (save/load/status over local persistence)
- Login/session and role-aware command gating

## 7) Team of 4 responsibilities

### Member 1 — Kernel & Boot
- Bootloader integration
- Kernel core
- Interrupts/drivers baseline
- Display/keyboard foundation

### Member 2 — Filesystem & Security
- Filesystem and persistence
- User accounts and login
- Roles and permissions

### Member 3 — Student Productivity
- Tasks, notes, timetable
- Dashboard
- XP/levels/achievements integration

### Member 4 — Smart & Advanced Features
- Smart Behavior Engine
- Voice command service
- StudyCloud simulation
- Developer tooling support

## 8) Development roadmap

1. Architecture  
2. Development setup  
3. Project structure  
4. Bootloader + kernel entry  
5. Basic kernel terminal  
6. Shell  
7. Filesystem  
8. Security  
9. Productivity suite  
10. Gamification  
11. Smart behavior  
12. Voice command path  
13. Developer tools  
14. StudyCloud simulation  
15. UI improvement  
16. Testing  
17. Final documentation and demo

## 9) First milestone

Produce a bootable ISO in QEMU and show:

`StudyOS Kernel Started Successfully!`

This milestone confirms:
- build toolchain works
- GRUB loads the kernel
- control successfully reaches kernel entry

---

## Initial architecture layers

```text
                    ┌──────────────────────────┐
                    │     STUDYOS UI/APPS      │
                    │ Shell, Dashboard, Tools  │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────▼─────────────┐
                    │    USER SPACE SERVICES   │
                    │ Productivity, Smart, XP, │
                    │ Voice Adapter, Cloud Sim │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────▼─────────────┐
                    │       SYSTEM CALLS       │
                    └────────────┬─────────────┘
                                 │
             ┌───────────────────▼──────────────────┐
             │                KERNEL                │
             │ Memory | Process | I/O | FS | Sec    │
             └───────────────────┬──────────────────┘
                                 │
                    ┌────────────▼─────────────┐
                    │         HARDWARE         │
                    │ CPU/RAM/Disk/Input       │
                    └──────────────────────────┘
```

## Initial project structure (Phase 1 skeleton)

```text
studyos/
├── boot/
├── kernel/
├── drivers/
├── filesystem/
├── security/
├── shell/
├── services/
│   ├── smart_behavior/
│   ├── productivity/
│   ├── gamification/
│   ├── voice/
│   └── cloud/
├── apps/
│   ├── notes/
│   ├── tasks/
│   ├── timetable/
│   ├── editor/
│   └── settings/
└── docs/
```
