# REPORT.md

## Assignment 2 — Custom `ls` Implementation

This report documents the design, implementation, and testing of the custom `lsv` utility through its first three feature versions.

---

## **Feature 1: Basic Build (v1.0.0)**

### 🧩 Description
This version implements the foundational build system and basic functionality of the `lsv` command. It lists non-hidden files and directories from the current working directory in a single column.

### ⚙️ Implementation Details
- The program opens the target directory using `opendir()` and iterates through entries with `readdir()`.
- Hidden files (those starting with `.`) are ignored.
- Filenames are printed separated by two spaces.
- A structured Makefile was created with `src`, `obj`, and `bin` directories for clean builds.
- Compilation uses `gcc` with flags `-Wall -Wextra -std=c11`.

### 🧪 Test Commands
```bash
make
./bin/lsv1.0.0
```

### ✅ Output Example
```
LICENSE  README.md  src  bin  obj  makefile
```

### 📘 Learning Outcome
Understanding of directory traversal using `dirent.h`, basic Makefile automation, and clean build practices.

---

## **Feature 2: Long Listing Format (-l) (v1.1.0)**

### 🧩 Description
This version introduces the `-l` option to display files in the long listing format, similar to the standard `ls -l` command.

### ⚙️ Implementation Details
- Added command-line parsing using `getopt()` to detect the `-l` option.
- Used `lstat()` to retrieve detailed file information.
- File attributes displayed include:
  - Type and permissions (via `print_permissions()`)
  - Link count (`st_nlink`)
  - Owner and group names (`getpwuid()`, `getgrgid()`)
  - File size (`st_size`)
  - Last modification time (`ctime()`)
- Proper formatting of permission bits and file types (`S_ISDIR`, `S_ISREG`, etc.)

### 🧪 Test Commands
```bash
make
./bin/lsv1.1.0 -l
```

### ✅ Output Example
```
drwxr-xr-x 2 umair umair 4096 Mon Oct  6 19:55:48 2025 src
-rw-r--r-- 1 umair umair  1072 Mon Oct  6 19:49:19 2025 LICENSE
```

### 📘 Learning Outcome
- Familiarity with POSIX file attributes.
- Use of system calls `lstat`, `getpwuid`, and `getgrgid`.
- Handling time formatting and output alignment.

---

## **Feature 3: Column Display (Down Then Across) (v1.2.0)**

### 🧩 Description
This version upgrades the default behavior (when no options are provided) to display directory contents in **multi-column format**, mimicking the default `ls` behavior.

### ⚙️ Implementation Details
- All filenames are read into a dynamically allocated array before printing.
- The maximum filename length is determined to calculate column width.
- Terminal width is obtained using `ioctl()` with `TIOCGWINSZ`.
- Number of columns and rows are calculated as:
  ```c
  columns = terminal_width / (max_len + spacing);
  rows = ceil(total_files / columns);
  ```
- The program prints files **down then across** using index mapping:
  ```c
  filenames[row + col * num_rows]
  ```
- Proper spacing is maintained between columns for alignment.

### 🧪 Test Commands
```bash
make
./bin/lsv1.2.0
./bin/lsv1.2.0 -l
```

### ✅ Output Example
```
LICENSE       README.md    bin
makefile      obj          src
```

### 📘 Report Questions

**Q1. Explain the general logic for printing items in a “down then across” columnar format. Why is a simple single loop insufficient?**  
A simple loop prints items sequentially, resulting in a single column. To print in a "down then across" manner, we must calculate both the number of columns and rows, then map each file's position based on its index relative to the row count. This ensures that the output fills columns evenly and adapts to terminal width.

**Q2. What is the purpose of the ioctl system call in this context?**  
The `ioctl()` system call with `TIOCGWINSZ` retrieves the terminal's current width in characters. Using this information allows dynamic adjustment of the number of columns. Without it, a fixed width (e.g., 80 chars) would limit adaptability, causing misalignment on different screen sizes.

### 🧩 Tag & Release Workflow
- Created branch `feature-column-display-v1.2.0`.
- Committed all changes and merged back into `main`.
- Tagged release:
  ```bash
  git tag -a v1.2.0 -m "Version 1.2.0 – Column Display (Down Then Across)"
  git push origin v1.2.0
  ```

---

### 🏁 Summary
| Version | Feature | Key Concept | Status |
|----------|----------|-------------|---------|
| v1.0.0 | Basic Build | Directory Reading, Makefile | ✅ Completed |
| v1.1.0 | Long Listing (-l) | File Metadata, Permissions | ✅ Completed |
| v1.2.0 | Column Display | Terminal I/O, Formatting | ✅ Completed |

---

**Instructor:** Dr. Muhammad Arif Butt  
**Student:** Muhammad Umair  
**Course:** Operating Systems — Assignment 2  
**Institution:** PUCIT (University of the Punjab College of Information Technology)
