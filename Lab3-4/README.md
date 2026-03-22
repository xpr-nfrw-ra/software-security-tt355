# Licensing System

A hardware-locked software licensing system written in C++ for Windows. Licenses are tied to both the machine they are created on and a specific program — they cannot be used on a different computer or for a different program.

---

## What It Does

- **Generates a unique machine fingerprint** from your CPU, motherboard serial number, and MAC address
- **Hashes the target program's executable** to create a program-specific identifier
- **Creates licenses** that are valid only on your machine and only for the specified program
- **Supports time-limited licenses** (1, 2, 7, 14, 30 days, or any custom number of days)
- **Supports perpetual licenses** (no expiry)
- **Saves licenses to files** that can be stored and verified later

---

## Building the Project

### Requirements
- Windows 10 or later
- CMake 3.10 or later
- Visual Studio Build Tools (any recent version)

### Steps

1. Open a terminal (Command Prompt or PowerShell) and navigate to the `Lab3-4` folder:
   ```
   cd path\to\lab3-project\Lab3-4
   ```

2. Create a build directory and enter it:
   ```
   mkdir build
   cd build
   ```

3. Configure the project:
   ```
   cmake ..
   ```

4. Build the project:
   ```
   cmake --build .
   ```

5. The executable will be located at:
   ```
   Lab3-4\build\Debug\LicensingSystem.exe
   ```

---

## Running the Program

From the `build` folder, run:
```
Debug\LicensingSystem.exe
```

When the program starts, it will display your machine's hardware identifiers and then show the main menu:

```
==================================================
    LICENSE SYSTEM - LABORATORY WORK
==================================================

Current Device ID: A1B2C3D4E5F60708
CPU ID: ...
MAC address: ...

--- SELECT ACTION ---
1. Create perpetual license
2. Create temporary license
3. Verify license
4. Show all licenses
5. Save license to file
0. Exit
```

---

## Step-by-Step Usage Guide

### Creating a Perpetual License

A perpetual license never expires.

1. Select option `1` from the menu
2. Enter the name of the licensee (the person or organization the license is issued to):
   ```
   Enter licensee name: John Doe
   ```
3. Enter a friendly name for the program being licensed:
   ```
   Enter program name (e.g., Visual Studio): Visual Studio
   ```
4. Enter the full path to the program's executable file:
   ```
   Enter program executable path: C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe
   ```

The license details will be displayed, including the generated license key.

---

### Creating a Temporary License

A temporary license expires after a set number of days.

1. Select option `2` from the menu
2. Enter the licensee name, program name, and executable path (same as above)
3. Choose a duration from the menu:
   ```
   Select license duration:
   1.  1 day
   2.  2 days
   3.  7 days
   4.  14 days
   5.  30 days
   6.  Custom
   ```
4. If you select `6` (Custom), enter the number of days:
   ```
   Enter number of days: 45
   ```

---

### Verifying a License

Checks whether a license key is valid for this machine and program.

1. Select option `3` from the menu
2. Enter the license key exactly as shown (format: `XXXXX-XXXXX-XXXXX`):
   ```
   Enter license key: 3A9F2-1C8E7-0D456
   ```
3. Enter the path to the program's executable:
   ```
   Enter program executable path: C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe
   ```

The program will report whether the license is valid or not, and if invalid, will explain why (wrong machine, wrong program, or expired).

---

### Showing All Licenses

Select option `4` to display all licenses created in the current session.

---

### Saving a License to a File

Saves a license to a `.txt` file so it can be stored or verified later.

1. Select option `5` from the menu
2. Enter the license key you want to save:
   ```
   Enter license key: 3A9F2-1C8E7-0D456
   ```
3. Enter the filename to save to (e.g., `license.txt`):
   ```
   Enter filename: license.txt
   ```

The file will be saved in the current directory and will look like this:
```
KEY=3A9F2-1C8E7-0D456
MACHINE=A1B2C3D4E5F60708
TYPE=PERPETUAL
PROGRAM_NAME=Visual Studio
PROGRAM_ID=FA3C9201B84E7D56
LICENSEE=John Doe
```

---

## Important Notes

- **Licenses are machine-specific.** A license created on one computer will not validate on another.
- **Licenses are program-specific.** A license for Visual Studio will not validate for any other program, and vice versa.
- **The program path must match exactly.** When verifying a license, provide the same executable path that was used when creating it.
- **In-memory only.** Licenses are lost when the program closes unless you save them to a file using option `5`.
