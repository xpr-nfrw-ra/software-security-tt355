# ProtectUSB

A command-line tool for encrypting and decrypting files on USB drives to protect against unauthorized copying. Built with C++17 for Windows.

## Features

- **AES-256-CBC Encryption** — Industry-standard encryption using the Windows BCrypt API. Each file gets a unique random IV.
- **Compression** — Optional XPRESS Huffman compression before encryption to reduce file sizes (Windows Compression API).
- **Parallel Processing** — Multi-threaded encryption/decryption for faster processing of multiple files.
- **USB & DVD Detection** — Automatically detect connected USB removable drives and DVD/CD-ROM drives.
- **Key from File** — Read the encryption key from a file instead of typing it interactively.
- **Chunked I/O** — 64KB streaming for large files without loading them entirely into memory.
- **Protected State Tracking** — A `config.dat` file tracks whether a drive has been encrypted, preventing accidental double-encryption.

## Requirements

- **OS:** Windows 10 or later
- **Compiler:** Visual Studio 2022 (MSVC v143, C++17)
- **Platform:** x64

No external libraries are required. The project uses only Windows system APIs:
- `bcrypt.lib` — AES-256-CBC encryption and SHA-256 key derivation
- `Cabinet.lib` — XPRESS Huffman compression/decompression

Both are linked automatically via `#pragma comment`.

## Building

### Visual Studio 2022

1. Open `Lab5.sln` in Visual Studio 2022
2. Select configuration: **Release | x64** (or Debug | x64)
3. Build the solution (**Ctrl+Shift+B**)
4. The executable will be in `x64/Release/ProtectUSB.exe` (or `x64/Debug/`)

## Usage

```
protectusb [command] [arguments] [options]
```

### Commands

| Command | Description |
|---|---|
| `scan` | Detect connected USB and DVD/CD-ROM drives |
| `list <path>` | List files in the specified directory |
| `protect <path>` | Encrypt all files on a USB drive |
| `unprotect <path>` | Decrypt all `.encrypted` files on a USB drive |
| `encrypt <file>` | Encrypt a single file |
| `decrypt <file>` | Decrypt a single file |
| `info <path>` | Show drive volume name and serial number |
| `help` | Show help message |

### Options

| Option | Description |
|---|---|
| `--key-file <path>` | Read the encryption key from a file (first line) instead of prompting |
| `--compress` | Compress files before encryption (reduces size, uses XPRESS Huffman) |
| `--parallel` | Process multiple files in parallel (multi-threaded) |

### Examples

**Scan for connected drives:**
```
protectusb scan
```

**Encrypt all files on a USB drive:**
```
protectusb protect D:\
```
You will be prompted to enter an encryption key.

**Encrypt with compression and parallel processing:**
```
protectusb protect D:\ --compress --parallel
```

**Encrypt using a key from a file:**
```
protectusb protect D:\ --key-file C:\keys\mykey.txt
```

**Decrypt all files on a USB drive:**
```
protectusb unprotect D:\
```

**Encrypt a single file with compression:**
```
protectusb encrypt document.pdf --compress
```

**Decrypt a single file:**
```
protectusb decrypt document.pdf.encrypted
```

**Show drive information:**
```
protectusb info D:\
```

## Encrypted File Format

### Version 2 (current)

```
[4 bytes: 'PUSB' magic] [1 byte: flags] [16 bytes: random IV] [encrypted data...]
```

- **Flags byte:** bit 0 = compressed (1 = XPRESS Huffman compression applied before encryption)
- **Encrypted data:** AES-256-CBC with PKCS7 padding
- When compressed, the plaintext payload is: `[8 bytes: original file size (LE)] [compressed data]`

### Version 1 (legacy, read-only)

```
[16 bytes: random IV] [encrypted data...]
```

Files encrypted with the previous version are automatically detected and can still be decrypted.

## Key File Format

The key file should contain the encryption password on the first line. Trailing whitespace is trimmed.

```
MySecretPassword123
```

## Project Structure

```
ProtectData-Project/
  Lab5-6/
    Lab5.sln                    # Visual Studio solution
    Lab5.vcxproj                # Project file
    src/
      main.cpp                  # CLI entry point, protect/unprotect logic
      cipher.cpp/h              # AESCipher - AES-256-CBC encrypt/decrypt
      compression.cpp/h         # Compression - XPRESS Huffman compress/decompress
      usb_detector.cpp/h        # USBDetector - USB and DVD drive detection
      file_operations.cpp/h     # FileOperations - file listing, collection, deletion
      utils.cpp/h               # Utils - password input, key file reading, helpers
  exe/
    ProtectUSB.exe              # Pre-built executable
  Doc/
    Lab5.docx                   # Documentation (Armenian)
    Lab5.pdf
```

## Security Notes

- Encryption uses AES-256-CBC with a random 16-byte IV per file
- Key derivation uses a single pass of SHA-256 (sufficient for educational use; production systems should use PBKDF2 or Argon2)
- Original files are deleted after successful encryption
- The `config.dat` file prevents accidental double-encryption of a drive

## License

University lab project. For educational purposes.
