# Prime Number Search

This project is a prime number search compiled for Linux. It uses multi-threading and Advanced Vector Instructions (AVX2) for divisibility testing.

## System Compatibility

- **Operating System:** Ubuntu 22.04.3 LTS
- **Linux Distribution Details:**
  ```
  PRETTY_NAME="Ubuntu 22.04.3 LTS"
  NAME="Ubuntu"
  VERSION_ID="22.04"
  VERSION="22.04.3 LTS (Jammy Jellyfish)"
  VERSION_CODENAME=jammy
  ID=ubuntu
  ID_LIKE=debian
  ```
- **Note:** This program relies on Linux-specific system calls and registers, it may not function properly in Windows.

## Configuration Parameters (config.txt)

The `config.txt` file contains the following configurable parameters:

### `threads` (Number of Threads)
- Specifies the number of threads used by the program.
- Must be a **positive integer**.
- Cannot exceed **half of the search range**; otherwise, it will be automatically adjusted.
- Must be an **even number**; odd values will be forced to be even.

### `ceiling` (Search Range Limit)
- Defines the upper limit for prime number searches.
- The output will include all prime numbers between and including **1** and the `ceiling`.
- The hard limit is **1,000,000,000**; values exceeding this will be rejected.
- Must be a **positive integer**.

### `mode` (Thread Dispatch Mode)
- Controls how threads are assigned tasks.
- Allowed values: `0` or `1`.
  - `0`: Threads receive subranges of numbers to test sequentially.
  - `1`: Threads receive a range of divisors for divisibility testing.

### `avx` (Advanced Vector Instructions)
- Determines if AVX2 will be used for divisibility testing.
- Allowed values: `0` (No AVX) or `1` (Use AVX).
- Will be forcibly disabled if `ceiling` **exceeds 16,000,000**.
- **Compilation Note:** Do **not** use `-O3` optimization, as it interferes with required registers.

### `print` (Output Method)
- Determines how primes are printed.
- Allowed values: `0` or `1`.
  - `0`: Immediate printing as numbers are found.
  - `1`: Delayed printing after all threads finish.

## Compilation Instructions

To compile the project, use the following commands:
```sh
nasm -f elf64 find_prime.asm -o find_prime.o
gcc -O0 main.c find_prime.o -o lily -lm
```
- `lily` is a placeholder; replace it with your desired executable name.

Alternatively, you can use the `run.sh` script to compile and run the program:
```sh
./run.sh
```

## Running the Program
After compilation, execute the program using:
```sh
./lily
```
- `lily` is a placeholder.
- The executable can be run without recompilation, even if `config.txt` is modified.

## Notes
- Ensure all required dependencies (GCC, NASM) are installed.
- The AVX2 instructions require a compatible CPU.
- The program has been optimized for Linux and may not work on other operating systems.

