# Lab 11 — Control-Flow Flattening + Anti-Debug / Anti-VM

## Purpose of the lab

Lab 11 covers two families of software obfuscation techniques:

1. **Argument Count Obfuscation** — hiding a function's real parameters behind dummy, packed, or variadic arguments.
2. **Code Scattering in Memory** — breaking the natural shape of a program so that static analysis tools and human reverse engineers cannot easily follow what it does.

The lab ends with four suggested advanced directions (VM-based obfuscator, control-flow flattening, polymorphic engine, anti-VM + anti-debugging). This project picks two of them — **control-flow flattening** and **anti-VM / anti-debugging** — and *fuses* them into a single demonstration, because the two techniques reinforce each other.

The artifact under test is a tiny **software license-key checker**. Protecting license checks is the canonical real-world scenario for these techniques: it is the place a cracker would point a debugger at, and it benefits the most from making "where does this code decide accept vs. reject?" hard to answer.

The goal is **not** to ship an unbreakable protection scheme. The goal is to demonstrate, end to end, that:

- A program's logic can be rewritten into a form that hides its natural control flow.
- Environment-detection probes can be wired *into* that hidden control flow so that detection silently corrupts execution — with no visible "debugger detected" branch for an attacker to find and patch.
- Both versions behave identically to a normal user on a normal machine.

## The two techniques, fused

**Control-Flow Flattening (CFG flattening).** Normal code has a shape — `if`s nest, loops have headers, functions return. A reverse engineer reads that shape to understand intent. Flattening destroys the shape: every basic block becomes a `case` in one big `switch`, and a single integer `state` variable drives the dispatcher. The function becomes:

```cpp
while (state != TERM) {
    switch (state) {
        case 0: /* ... */ next = 1; break;
        case 1: /* ... */ next = 2; break;
        // ...
    }
    state = next;
}
```

All blocks look structurally identical. Order is gone.

**Anti-debug / anti-VM probes.** Standard environment-detection tricks: `IsDebuggerPresent`, an `rdtsc` timing check (single-stepping inflates the delta), the CPUID "hypervisor present" bit, and similar. Each probe returns `0` (clean) or `1` (detected).

**The fusion.** Instead of writing `if (probe) exit();` — which is one line a cracker can find and `NOP` out — the probe result is XOR-mixed into the next-state computation of the flattened dispatcher:

```cpp
state = next ^ (int)probe_mask();
```

When the environment is clean, `probe_mask()` returns `0`, so `state == next` and the program runs normally. Under a debugger or VM, `probe_mask()` returns a non-zero constant, `state` jumps to a number that has no matching `case`, the `default:` arm fires, and the function terminates with `result = 0`. There is no visible "detected" branch, no error string, no obvious place to patch.

## Key format

The license key is `XXXX-XXXX-XXXX` — three groups of 4 hex digits joined by `-`. It is valid iff:

- length is exactly 14;
- characters at indices 4 and 9 are `-`;
- the other 12 characters are hex digits;
- the sum of all 12 nibbles, mod 16, equals `0xA`;
- the three 16-bit groups XOR to `0xBEEF`.

A working sample key is **`BEEF-000A-000A`**.

## Files

| File              | Role                                                                    |
|-------------------|-------------------------------------------------------------------------|
| `baseline.cpp`    | Honest, linear implementation of the key check.                         |
| `anti_probes.h`   | The probe library: `IsDebuggerPresent`, `rdtsc` timing, CPUID hypervisor. |
| `generate.py`     | Block list + code generator that emits the flattened C++.               |
| `obfuscated.cpp`  | Generated. Do not edit by hand; rerun `generate.py`.                    |
| `README.md`       | This file.                                                              |

## Step-by-step walkthrough

Run each step from `C:\Users\Lenovo\Desktop\lab11-dev\Lab11` in PowerShell. Requires g++ (tested with MSYS2 MinGW-w64) and Python 3.

### Step 1 — Inspect the project files

**What this shows.** The full set of inputs (sources, header, generator) and outputs (binaries, generated C++).

```powershell
ls
```

Expected output:
```
anti_probes.h
baseline.cpp
baseline.exe
generate.py
obfuscated.cpp
obfuscated.exe
obfuscated_strict.exe
README.md
```

### Step 2 — Read the honest source

**What this shows.** A normal, linear key-check function. The logic is easy to follow: a few `if`s and a `for` loop. This is the version a reverse engineer *wishes* they had.

```powershell
cat baseline.cpp
```

### Step 3 — Run the honest binary

**What this shows.** The baseline accepts the valid key and rejects everything else. Establishes the "correct" behavior the obfuscated version must preserve.

```powershell
.\baseline.exe BEEF-000A-000A
.\baseline.exe nope
.\baseline.exe DEAD-BEEF-1234
```

Expected output:
```
OK
BAD
BAD
```

### Step 4 — Read the generator and regenerate the obfuscated source

**What this shows.** The block list IR (one entry per basic block of the original function) and the emitter that turns it into a flattened `while/switch` dispatcher. Note the critical line near the bottom: `state = next ^ (int)probe_mask();` — this is where the two techniques fuse.

```powershell
cat generate.py
python generate.py > obfuscated.cpp
```

(No output from the second command — it just rewrites `obfuscated.cpp`.)

### Step 5 — Read the generated obfuscated source

**What this shows.** The same key check from `baseline.cpp`, reshaped into a flat state machine. Every block looks structurally identical; the program's natural shape is gone.

```powershell
cat obfuscated.cpp
```

### Step 6 — Build the obfuscated binary (default flags)

**What this shows.** A clean compile with the default probe set (debugger + timing). The hypervisor probe is opt-in, see step 8.

```powershell
g++ -O2 -o obfuscated.exe obfuscated.cpp
```

Expected output: none (silent success).

### Step 7 — Verify the obfuscated binary is behavior-preserving

**What this shows.** On a normal clean run, `probe_mask()` returns 0, so `state == next` and the flattened version produces exactly the same answers as the baseline. The end user notices nothing.

```powershell
.\obfuscated.exe BEEF-000A-000A
.\obfuscated.exe nope
.\obfuscated.exe DEAD-BEEF-1234
```

Expected output:
```
OK
BAD
BAD
```

### Step 8 — Build a stricter variant with the hypervisor probe enabled

**What this shows.** How the probe set is extended at build time. The hypervisor probe is gated behind `ENABLE_HYPERVISOR_PROBE` because Windows 11 hosts with Hyper-V / VBS enabled set the CPUID hypervisor bit even on bare metal, which causes false positives. Turning it on simulates "the obfuscated program decided the environment is hostile."

```powershell
g++ -O2 -DENABLE_HYPERVISOR_PROBE=1 -o obfuscated_strict.exe obfuscated.cpp
```

Expected output: none (silent success).

### Step 9 — Demonstrate probe derailment

**What this shows.** The headline result. Same valid key, same source code, same compiler, only difference is whether a probe fires:

- `baseline.exe` — no probes, accepts the key.
- `obfuscated.exe` — probes do not fire on this host, accepts the key.
- `obfuscated_strict.exe` — hypervisor probe fires, `probe_mask()` returns non-zero, `state` jumps to an undefined case, `default:` sets `result = 0`, output is `BAD`.

There is no `if (detected) reject;` line in the source. The rejection is a *consequence* of state-machine arithmetic.

```powershell
.\baseline.exe          BEEF-000A-000A
.\obfuscated.exe        BEEF-000A-000A
.\obfuscated_strict.exe BEEF-000A-000A
```

Expected output:
```
OK
OK
BAD
```

## Why the fusion matters

If you wrote:

```cpp
if (IsDebuggerPresent()) { result = 0; goto end; }
```

an attacker finds the call, replaces the conditional jump with two `NOP`s, and the protection is gone in under a minute. The string `IsDebuggerPresent` is even visible in the binary.

When the probe result is folded into a state-machine transition, there is no single instruction to neutralize. The dispatcher loop looks the same whether or not detection happened — the difference is *which integer ends up in `state` next*. To defeat it, the attacker has to understand the entire flattened control flow, the meaning of each state, and the probe interaction — which is exactly the work the obfuscation is intended to impose.

## Scope and limitations

This is a teaching artifact, not a hardened protection scheme. A motivated reverse engineer would defeat this in minutes — the probes can be NOP'd at their call sites, the dispatcher can be lifted with symbolic execution, and the block list is small enough to recover by hand. The educational point is the *construction*, not its strength.
