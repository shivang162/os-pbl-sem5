# StudyOS Phase 8 — Security System

## Educational Scope

StudyOS Security is an educational demonstration of
authentication, authorization, sessions, and permissions.

It is NOT intended to provide production-level OS security.

## 1) Authentication

- Boot now requires login before shell access.
- `login` command is available for manual login attempts.
- Invalid credentials return a generic failure message without revealing which field was incorrect.
- Login attempts are limited to 3 per prompt cycle.

## 2) Authorization

- Role-based checks are enforced through reusable permission checks.
- Admin commands require `ROLE_ADMIN`.
- Unauthorized access returns clear permission denied messages.

## 3) Users

- Users are managed in an in-memory user table (fixed-size educational model).
- Admin commands:
  - `users` (list users)
  - `useradd` (create user)
  - `userdel <username>` (delete user with confirmation)

## 4) Roles

- `USER`
- `ADMIN`

`ADMIN` includes all `USER` capabilities plus user management commands.

## 5) Password Storage

- Passwords are not stored as plain text.
- A simple educational hash is used for storage and verification.
- In production, this should be replaced with a modern password KDF such as Argon2id, bcrypt, or scrypt.

## 6) Sessions

- Session tracks:
  - logged-in status
  - current username
  - current role
- `logout` clears the active session and requires re-authentication.
- Reboot resets session state because all data is in-memory.

## 7) Filesystem Permissions

- Filesystem entries now include owner metadata.
- Regular users can access only their own files.
- Admin can access all files.
- Creation under `/home/<username>/...` is restricted to that user (or admin).

## 8) Security Limitations

- No persistent user database yet.
- Hash function is educational, not cryptographically hardened.
- No timed lockout/backoff mechanism after failed attempts.
- No MFA, audit log persistence, or secure hardware-backed secrets.

## 9) Test Cases (Phase 8)

Validated scenarios:

1. Successful USER login (`student/student123`)
2. Successful ADMIN login (`admin/admin123`)
3. Wrong password rejection
4. USER denied on admin command `users`
5. ADMIN access to `users`
6. Logout clears session and prompts for login
7. Access denied behavior when not authenticated
8. File protection between users (including `/home/admin/private.txt`)
9. Password change via `passwd`
10. Reboot requires login again (no persisted session)

## 10) Future Improvements

- Persistent user and permission storage
- Strong password hashing (Argon2id/bcrypt/scrypt)
- Better lockout policy with time-based throttling
- Group-based and file-level ACL permissions
- Security event logging and monitoring
