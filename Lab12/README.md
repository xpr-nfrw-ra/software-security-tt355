# Lab 12 — Reverse Engineering

This lab demonstrates two foundational reverse-engineering topics in C++:

1. A toy **executable packer** (XOR-based) with a matching unpacking **stub**.
2. A simple **crackme** with an obfuscated password check, plus a **solver** that recovers the password.

Two anti-debugging extensions are added to the crackme on top of the baseline from `Lab12.docx`.

---

## Project layout

```
lab12-dev/
├── Lab12.docx          # original assignment (Armenian)
├── README.md           # this file
├── CLAUDE.md           # AI-context notes
└── Lab12/
    ├── packer.cpp      # XOR-encrypts program.exe → packed.bin
    ├── stub.cpp        # decrypts packed.bin → unpacked.exe and runs it
    ├── crackme.cpp     # password-check program (with anti-debug)
    ├── solver.cpp      # recovers the crackme password
    ├── Makefile        # `make` (if GNU make installed)
    └── build.bat       # Windows fallback build script
```

---

## File-by-file explanation

### `packer.cpp` — the packer
Reads `program.exe` from disk, XOR-encrypts every byte with the key `0xAA`, and writes the result to `packed.bin`. This simulates what a real packer (e.g. UPX) does at the most basic level — obscuring the original bytes so the file no longer looks like a normal executable.

Key steps:
- `read_file()` opens the file in binary mode, seeks to end to learn the size, then reads everything into a `vector<char>`.
- `xor_encrypt()` XORs each byte in place with `0xAA`.
- `write_file()` writes the encrypted bytes back out as `packed.bin`.

### `stub.cpp` — the unpacker / runner
The companion program. It reads `packed.bin`, runs the same XOR with `0xAA` (XOR is self-inverse, so encrypting twice = decrypting), writes the recovered bytes as `unpacked.exe`, and finally launches `unpacked.exe` via `system(".\\unpacked.exe")`.

After running, `unpacked.exe` is byte-for-byte identical to the original `program.exe`.

### `crackme.cpp` — the crackme (with anti-debug extensions)
Asks the user for a 5-character password and validates it with this obfuscated check:

```
for i in 0..5: (input[i] XOR 0x55) == key[i]
key = {72, 29, 7, 0, 91}
```

The real password is *not* stored anywhere directly — only the XOR-encoded form is. A reverse engineer would have to find the key array and the XOR constant by reading the disassembly, then invert the operation.

#### Anti-debugging extensions added on top of the docx baseline

Two layers (both listed in the docx as "advanced ideas"):

1. **`IsDebuggerPresent()`** — a Windows API call that returns true when the current process is being debugged. The crackme exits immediately if a debugger is attached. This is the classic first line of defence.
2. **Timing check** — runs a trivial loop and measures it with `std::chrono`. A debugger that is single-stepping or has heavy instrumentation makes simple operations take orders of magnitude longer. If the loop exceeds 5000 µs, the program assumes it is being analysed and exits.

Together these stop a casual debugger attach. A determined attacker can patch them out, of course — that is exactly the point of the exercise.

### `solver.cpp` — the solver
Performs the inverse of the crackme's check: for each byte in the key array, computes `key[i] XOR 0x55` and prints the result. The output is the real password.

Note: with the docx's chosen key, two of the five password bytes are **non-printable** (`0x1D` and `0x0E`), so the password cannot be typed at a keyboard directly. To verify the crackme, pipe the solver's output into it (see "Verifying the crackme" below).

---

## Building

### With g++ (MinGW / MSYS2 UCRT64)

```cmd
cd Lab12
build.bat
```

Or manually:
```cmd
g++ -std=c++17 packer.cpp  -o packer.exe
g++ -std=c++17 stub.cpp    -o stub.exe
g++ -std=c++17 crackme.cpp -o crackme.exe
g++ -std=c++17 solver.cpp  -o solver.exe
```

### With GNU make
```
make
```

> Note: built without `-O2`. The original `istreambuf_iterator` read pattern from the docx interacted poorly with the optimiser on this MinGW build, so the file-reading helper was switched to the more reliable `seek-to-end + read` pattern.

---

## Running

### Packer / stub demo

1. Place any small Windows executable in `Lab12/` and rename it to `program.exe` (for testing, copying `solver.exe` to `program.exe` works fine).
2. Run the packer:
   ```
   packer.exe
   ```
   → produces `packed.bin`. Opening it in a hex editor shows the bytes are scrambled.
3. Run the stub:
   ```
   stub.exe
   ```
   → produces `unpacked.exe` and runs it. The output of the original program appears.

You can verify the roundtrip is lossless:
```
fc /b program.exe unpacked.exe
```

### Crackme demo

Wrong password:
```
crackme.exe
Enter password: hello
Access Denied!
```

Correct password — pipe the solver's bytes in (because the password contains non-printable characters):
```cmd
solver.exe > pw.txt
... (strip the "Recovered password: " prefix, then) ...
type pw.txt | crackme.exe
```

On a POSIX-style shell (Git Bash / MSYS):
```
./solver.exe | sed 's/Recovered password: //' | ./crackme.exe
→ Enter password: Access Granted!
```

### Anti-debug demo

Run `crackme.exe` from a debugger (e.g. attach `x64dbg` or `gdb`). The program will print `Debugger detected. Exiting.` and quit before asking for a password. Even without an attached debugger, if heavy instrumentation slows down the warm-up loop past 5 ms, the timing branch fires instead with `Suspicious timing. Exiting.`

---

## Conclusion

This lab walks through the building blocks behind two well-known reverse-engineering scenarios:

- **Packers** hide a program's real bytes until runtime, when a stub restores and executes them. Real packers (UPX, ASPack, Themida) extend the same pattern with compression, PE-header rewriting, in-memory loading, and self-decryption — but the conceptual core is what `packer.cpp` + `stub.cpp` show here.
- **Crackmes** train the reverse-engineer's eye for finding obfuscated comparisons, recovering keys, and inverting transformations. The XOR check in `crackme.cpp` is the simplest possible version; obfuscated commercial software stacks dozens of such tricks.
- **Anti-debugging** raises the cost of dynamic analysis. `IsDebuggerPresent()` is the most basic check; the timing approach catches debuggers that hide themselves from that flag. Both can be defeated by patching the binary — which is itself a reverse-engineering exercise.
