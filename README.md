



## Assignment 2 — Custom `ls` Implementation


This report documents the design, implementation, and testing of the custom `lsv` utility through its first four feature versions.



---



## Feature 1: Basic Build (v1.0.0)



### 🧩 Description

This version implements the foundational build system and basic functionality of the `lsv` command. It lists non-hidden files and directories from the current working directory in a single column.



### ⚙️ Implementation Details

* The program opens the target directory using `opendir()` and iterates through entries with `readdir()`.
* Hidden files (those starting with `.`) are ignored.
* Filenames are printed separated by two spaces.
* A structured Makefile was created with `src`, `obj`, and `bin` directories for clean builds.
* Compilation uses `gcc` with flags `-Wall -Wextra -std=c11`.



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


**## **Feature 2: Long Listing Format (-l) (v1.1.0)**



### 🧩 Description

This version introduces the `-l` option to display files in the long listing format, similar to the standard `ls -l` command.



### ⚙️ Implementation Details

* Added command-line parsing using `getopt()` to detect the `-l` option.
* Used `lstat()` to retrieve detailed file information.
* File attributes displayed include:

  * Type and permissions (via `print_permissions()`)
  * Link count (`st_nlink`)
  * Owner and group names (`getpwuid()`, `getgrgid()`)
  * File size (`st_size`)
  * Last modification time (`ctime()`)
* Proper formatting of permission bits and file types (`S_ISDIR`, `S_ISREG`, etc.)


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

* Familiarity with POSIX file attributes.
* Use of system calls `lstat`, `getpwuid`, and `getgrgid`.
* Handling time formatting and output alignment.



---



## Feature 3: Column Display (Down Then Across) (v1.2.0)



### 🧩 Description

This version upgrades the default behavior (when no options are provided) to display directory contents in **multi-column format**, mimicking the default `ls` behavior.



### ⚙️ Implementation Details

* All filenames are read into a dynamically allocated array before printing.
* The maximum filename length is determined to calculate column width.
* Terminal width is obtained using `ioctl()` with `TIOCGWINSZ`.
* Number of columns and rows are calculated as:

```c
columns = terminal_width / (max_len + spacing);
rows = ceil(total_files / columns);
```

* The program prints files **down then across** using index mapping:

```c
filenames[row + col * num_rows]
```

* Proper spacing is maintained between columns for alignment.



### 🧪 Test Commands

```bash
make
./bin/lsv1.2.0
./bin/lsv1.2.0 -l
```

\

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

* Created branch `feature-column-display-v1.2.0`.
* Committed all changes and merged back into `main`.
* Tagged release:

```bash
git tag -a v1.2.0 -m "Version 1.2.0 – Column Display (Down Then Across)"
git push origin v1.2.0
```



---


## Feature 4: Horizontal Column Display (-x) (v1.3.0)



### 🧩 Description

This version introduces the `-x` option for horizontal (row-major) column layout. Files are printed left to right, wrapping to the next line when the current line is full. This contrasts with the default "down then across" layout.



### ⚙️ Implementation Details

* Extended command-line parsing using `getopt()` to detect the `-x` option.
* Added a display mode flag to manage -l, -x, and default behaviors.
* All filenames are read into a dynamically allocated array.
* Maximum filename length and terminal width are calculated.
* Files are printed horizontally:

  1. Loop through filenames sequentially.
  2. Print each name padded to column width.
  3. Track horizontal position; if the next file exceeds terminal width, insert newline.



### 🧪 Test Commands

```bash
make
./bin/lsv1.3.0
./bin/lsv1.3.0 -l
./bin/lsv1.3.0 -x
```



### ✅ Output Example

```
LICENSE  README.md  bin  makefile  obj  src
```



### 📘 Report Questions

**Q1. Compare the complexity of "down then across" vs. "across" printing logic.**

"Down then across" requires pre-calculation of rows and careful index mapping to fill columns, making it more complex. "Across" is simpler, printing sequentially while checking horizontal width.

**Q2. How does the program manage different display modes?**

A display mode flag tracks the current mode (-l, -x, or default). The main function calls the corresponding print function based on this flag.



### 🧩 Tag & Release Workflow

* Created branch `feature-horizontal-display-v1.3.0`.
* Committed changes and merged into `main`.
* Tagged release:

```bash
git tag -a v1.3.0 -m "Version 1.3.0 – Horizontal Column Display (-x)"
git push origin v1.3.0
```

---



### 🏁 Summary

| Version | Feature                 | Key Concept                         | Status      |
| ------- | ----------------------- | ----------------------------------- | ----------- |
| v1.0.0  | Basic Build             | Directory Reading, Makefile         | ✅ Completed |
| v1.1.0  | Long Listing (-l)       | File Metadata, Permissions          | ✅ Completed |
| v1.2.0  | Column Display          | Terminal I/O, Formatting            | ✅ Completed |
| v1.3.0  | Horizontal Display (-x) | Output Formatting, State Management | ✅ Completed |

---

**Instructor:** Dr. Muhammad Arif Butt\
**Student:** Muhammad Umair\
**Course:** Operating Systems — Assignment 2\
**Institution:** PUCIT (University of the Punjab College of Information Technology)
