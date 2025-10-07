# Operating Systems Programming Assignment 2

**Student:** Muhammad Umair
**Roll No:** BSDSF23M006
**Course:** Operating Systems
**Instructor:** Muhammad Arif Butt

---

## Assignment Overview

This assignment focuses on re-engineering the `ls` utility from scratch in C. You will implement incremental features, including long listing, column display, horizontal display, alphabetical sort, colorized output, and recursive listing. The assignment emphasizes:

* System call programming (`stat()`, `lstat()`, `getpwuid()`, `getgrgid()`)
* Dynamic memory management and sorting (`qsort()`)
* Command-line argument parsing (`getopt()`)
* Terminal I/O and output formatting
* Git workflow and versioned releases

---

## Project Structure

```
ROLL_NO-OS-A02/
├── src/
│   └── ls-v1.0.0.c
├── bin/
├── obj/
├── man/
├── Makefile
├── README.md
└── REPORT.md
```

---

# Feature 1: Project Setup and Initial Build

**Tasks Completed:**

* Created GitHub repository `ROLL_NO-OS-A02`
* Cloned starter code `ls-v1.0.0` into `src/`
* Created required directories: `bin/`, `obj/`, `man/`
* Created empty `REPORT.md`
* Built and tested initial code
* Committed initial setup with message:

```
feat: Initial project setup with starter code
```

**Marks:** 5/5

---

# Feature 2: ls-v1.1.0 – Complete Long Listing Format (-l)

**Tasks Completed:**

* Created branch `feature-long-listing-v1.1.0`
* Implemented `getopt()` to parse `-l` flag
* Added function `display_long_listing()` using:

  * `stat()` / `lstat()` for file metadata
  * `getpwuid()` / `getgrgid()` for username/group
  * `ctime()` for formatted modification time
  * Bitwise operations on `st_mode` for permissions

**Report Questions and Answers:**

1. **Difference between `stat()` and `lstat()`**:

   * `stat()` follows symbolic links and returns info of the target file.
   * `lstat()` returns info about the link itself, not the target.
   * **Usage in ls**: `lstat()` is preferred when you want to show symbolic links instead of following them.

2. **Extracting file type and permissions from `st_mode`**:

```c
if (S_ISDIR(st.st_mode)) { /* Directory */ }
if (S_ISREG(st.st_mode)) { /* Regular file */ }
if (st.st_mode & S_IRUSR) { /* Owner read permission */ }
if (st.st_mode & S_IWGRP) { /* Group write permission */ }
```

Bitwise AND (`&`) with macros like `S_IFDIR`, `S_IRUSR`, etc., extracts type and permission info.

**Marks:** 15/15

---

# Feature 3: ls-v1.2.0 – Column Display (Down Then Across)

**Tasks Completed:**

* Created branch `feature-column-display-v1.2.0`
* Gathered all filenames into dynamic array
* Determined terminal width using `ioctl()`
* Calculated columns: `num_cols = terminal_width / (max_filename_length + 2)`
* Implemented "down then across" printing

**Report Questions and Answers:**

1. **Why is a single loop insufficient?**

   * Single loop prints filenames linearly; "down then across" requires row-major ordering. Pre-calculation of rows/columns ensures proper alignment.

2. **Purpose of `ioctl`**:

   * Detect terminal width dynamically.
   * Limitation of fixed width: Columns may overflow or underflow screen, reducing readability.

**Marks:** 15/15

---

# Feature 4: ls-v1.3.0 – Horizontal Column Display (-x)

**Tasks Completed:**

* Created branch `feature-horizontal-display-v1.3.0`
* Added `-x` flag in `getopt()`
* Implemented horizontal display function: row-major, wraps based on terminal width

**Report Questions and Answers:**

1. **Implementation complexity comparison:**

   * "Down then across" requires calculating number of rows first; horizontal is simpler, prints left-to-right and wraps when width exceeded.

2. **Display mode strategy:**

   * Enum or integer flag tracks mode (`long_list`, `horizontal`, `vertical`).
   * Based on flag, correct display function is called in `do_ls()`.

**Marks:** 15/15

---

# Feature 5: ls-v1.4.0 – Alphabetical Sort

**Tasks Completed:**

* Created branch `feature-alphabetical-sort-v1.4.0`
* Read all filenames into dynamic array
* Sorted array using `qsort()` and helper comparison function:

```c
int cmp(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}
```

**Report Questions and Answers:**

1. **Why read all entries before sorting?**

   * Sorting requires full list in memory. Drawback: High memory use for directories with millions of files.

2. **Purpose of comparison function in `qsort()`**:

   * Provides logic to compare two array elements.
   * Signature: `int cmp(const void *, const void *)` because `qsort()` works with generic `void *` pointers.

**Marks:** 15/15

---

# Feature 6: ls-v1.5.0 – Colorized Output Based on File Type

**Tasks Completed:**

* Created branch `feature-colorized-output-v1.5.0`
* Implemented ANSI color codes for different file types:

  * Directory: Blue (`\033[0;34m`)
  * Executable: Green (`\033[0;32m`)
  * Tarballs: Red (`\033[0;31m`)
  * Symbolic Links: Pink
  * Special files: Reverse video
* Used `stat()` / `lstat()` to determine file type

**Report Questions and Answers:**

1. **How ANSI escape codes work:**

   * Codes start with `\033[` (escape character), followed by code (`0;32m` for green), reset with `\033[0m`.

2. **Executable permission bits:**

   * Owner: `S_IXUSR`
   * Group: `S_IXGRP`
   * Others: `S_IXOTH`

**Marks:** 10/10

---

# Feature 7: ls-v1.6.0 – Recursive Listing (-R)

**Tasks Completed:**

* Created branch `feature-recursive-listing-v1.6.0`
* Added `-R` flag in `getopt()`
* Modified `do_ls()` to:

  * Print directory header
  * Read and sort entries
  * Display entries
  * Recursively call `do_ls()` for subdirectories (excluding `.` and `..`)
  * Construct full path: `dirname/entryname`

**Report Questions and Answers:**

1. **Base case in recursion:**

   * Stops recursion when there are no subdirectories to traverse or `.`/`..` is encountered.

2. **Why construct full path:**

   * Required to access nested directories correctly. Without it, `do_ls("subdir")` would fail if current working directory is different from parent.

**Marks:** 20/20

---

# Final Submission

* Final `REPORT.md` completed with all answers
* All feature branches merged into `main`
* GitHub repository contains all commits, tags, and release binaries

---
