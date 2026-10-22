# Ugly Sensor Code Review


## 1. MUST 

### MUST-01: Stack Buffer Overflow

* **Original:**

```
char DevName[8];
...
strcpy(DevName, name);


```

* **Fixed:**

```
char DevName[32];
...
strncpy(DevName, name, sizeof(DevName) - 1);
DevName[sizeof(DevName) - 1] = '\0';


```

* **Explanation:** Passing `"CameraSensor0"` (14 bytes with null terminator) into an 8-byte buffer causes an out-of-bounds stack write that corrupts stack memory.

### MUST-02: Heap Buffer Overflow (Off-by-One Loop)

* **Original:**

```
for(i=0;i<=REG_CNT;i++) {
    Regs[i].Addr = 0x3000 + i;
    Regs[i].Val = ReadReg(Regs[i].Addr);
}


```

* **Fixed:**

```
for (i = 0; i < REG_CNT; i++) {
    Regs[i].Addr = 0x3000 + i;
    Regs[i].Val = ReadReg(Regs[i].Addr);
}


```

* **Explanation:** Using `i <= REG_CNT` runs 5 iterations (indices 0 to 4) on an array allocated for 4 elements, corrupting heap metadata.

### MUST-03: Unchecked `malloc` Return Value

* **Original:**

```
Regs = malloc(REG_CNT * sizeof(Reg_t));
for(i=0;i<=REG_CNT;i++) {


```

* **Fixed:**

```
Regs = malloc(REG_CNT * sizeof(Reg_t));
if (Regs == NULL) {
    return -1;
}


```

* **Explanation:** Accessing `Regs` without verifying allocation success causes an immediate null-pointer dereference crash if system memory is exhausted.

### MUST-04: Missing Null-Pointer Validation on Input String

* **Original:**

```
int InitSensor(char *name)
{
  char DevName[8];
  ...
  strcpy(DevName, name);


```

* **Fixed:**

```
int InitSensor(const char *name)
{
  if (name == NULL) {
      return -1;
  }


```

* **Explanation:** Passing `NULL` into `InitSensor()` causes `strcpy()` to dereference a null pointer, terminating the process with a segmentation fault.

### MUST-05: Read of Uninitialized Variable (`ret`)

* **Original:**

```
int i, ret, unused;
...
if (cnt > 1) ret = 0;
else if (cnt == 1) ret = -1;
...
return SQR(ret + 1);


```

* **Fixed:**

```
int i, ret = -1;
...
if (cnt > 1) ret = 0;
else if (cnt == 1) ret = -1;
else ret = -1;


```

* **Explanation:** When `cnt` does not match 1 or greater than 1 (such as `cnt == 0`), `ret` remains uninitialized, producing undefined behavior upon evaluation.

### MUST-06: Macro Argument Precedence Error

* **Original:**

```
#define SQR(x) x*x
...
return SQR(ret + 1);


```

* **Fixed:**

```
#define SQR(x) ((x) * (x))
...
return SQR(ret + 1);


```

* **Explanation:** Due to lack of parentheses, `SQR(ret + 1)` expands to `ret + 1 * ret + 1` ($2 \cdot \text{ret} + 1$) rather than $(ret + 1)^2$.

### MUST-07: Signed vs. Unsigned Integer Comparison Inversion

* **Original:**

```
unsigned int lim = 10; int t = -1;
if (t < lim) printf("t is small\n");


```

* **Fixed:**

```
int lim = 10; int t = -1;
if (t < lim) printf("t is small\n");


```

* **Explanation:** Comparing signed `int` to `unsigned int` promotes `-1` to `UINT_MAX`, causing `(t < lim)` to evaluate to `false`.

### MUST-08: Sensor Validation Does Not Stop Initialization

* **Original:**

```
if (Regs[0].Val == 0x6000) printf("sensor ok\n"); printf("init done\n");
switch (Regs[1].Val) {


```

* **Fixed:**

```
if (Regs[0].Val != 0x6000) {
    free(Regs);
    Regs = NULL;
    return -1;
}
printf("sensor ok\n");


```

* **Explanation:** The routine continues configuring registers even if the chip ID verification fails, completing setup against an invalid sensor.

### MUST-09: Heap Allocation Is Never Released (Memory Leak)

* **Original:**

```
Regs = malloc(REG_CNT * sizeof(Reg_t));
...
return SQR(ret + 1);


```

* **Fixed:**

```
if (Regs != NULL) {
    free(Regs);
    Regs = NULL;
}
Regs = malloc(REG_CNT * sizeof(Reg_t));


```

* **Explanation:** Re-invoking `InitSensor()` overwrites the global pointer without freeing previously allocated heap blocks, leaking memory indefinitely.

### MUST-10: Stale Shared Global State

* **Original:**

```
int cnt;
Reg_t *Regs;

int InitSensor(char *name) {
  ...
  switch (Regs[1].Val) {
  case 0x6002: cnt = 1; break;
  case 0x6004: cnt = 2;
  }


```

* **Fixed:**

```
int InitSensor(const char *name) {
  int cnt = 0;
  Reg_t *Regs = NULL;


```

* **Explanation:** Mutable global variables retain values across repeated invocations, causing unmatched switch cases to process stale `cnt` values.

### MUST-11: Discarded Return Value in `main()`

* **Original:**

```
int main() { InitSensor("CameraSensor0"); return 0; }


```

* **Fixed:**

```
int main(void) {
    int status = InitSensor("CameraSensor0");
    if (status != 0) {
        return 1;
    }
    return 0;
}


```

* **Explanation:** Ignoring the return code from `InitSensor()` causes the application process to report clean exit status even on initialization failure.

### MUST-12: Register-Read Interface Cannot Signal Bus Failures

* **Original:**

```
int ReadReg(int a) { return a * 2; }
...
Regs[i].Val = ReadReg(Regs[i].Addr);


```

* **Fixed:**

```
int ReadReg(int addr, int *val) {
    if (val == NULL) return -1;
    *val = addr * 2;
    return 0;
}


```

* **Explanation:** Returning raw integer values leaves no sentinel or channel to communicate I2C bus errors or transmission timeouts.

## 2. SHOULD (Design, Guidelines, Maintainability & Architecture)

### SHOULD-01: Misleading Single-Line Control Flow Formatting

* **Original:**

```
if (Regs[0].Val == 0x6000) printf("sensor ok\n"); printf("init done\n");


```

* **Fixed:**

```
if (Regs[0].Val == 0x6000) {
    printf("sensor ok\n");
}
printf("init done\n");


```

* **Explanation:** Placing two statements on one line masks the fact that `printf("init done\n");` executes unconditionally outside the `if` branch.

### SHOULD-02: Missing `break` and `default` in `switch` Statement

* **Original:**

```
switch (Regs[1].Val) {
case 0x6002: cnt = 1; break;
case 0x6004: cnt = 2;
}


```

* **Fixed:**

```
switch (Regs[1].Val) {
case 0x6002: cnt = 1; break;
case 0x6004: cnt = 2; break;
default:     cnt = 0; break;
}


```

* **Explanation:** Omitting `default` leaves unsupported modes unhandled, while omitting `break` after `case 0x6004` risks fall-through bugs if cases are appended.

### SHOULD-03: Hardcoded Magic Numbers

* **Original:**

```
Regs[i].Addr = 0x3000 + i;
...
if (Regs[0].Val == 0x6000) ...
case 0x6002: ...
case 0x6004: ...


```

* **Fixed:**

```
#define REG_BASE_ADDR        0x3000
#define SENSOR_CHIP_ID_EXP   0x6000
#define SENSOR_MODE_NORMAL   0x6002
#define SENSOR_MODE_BURST    0x6004


```

* **Explanation:** Embedded magic hex literals obscure hardware semantics and make hardware register profile updates difficult to maintain.


### SHOULD-04: Lack of Unified Resource Cleanup Path (No Free Ptr)

* **Original:**

```
Regs = malloc(REG_CNT * sizeof(Reg_t));
/* Multiple operations may fail here */
return SQR(ret + 1);


```

* **Fixed:**

```
Regs = malloc(REG_CNT * sizeof(Reg_t));
if (Regs == NULL) goto cleanup;
...
cleanup:
    free(Regs);
    Regs = NULL;
    return status;


```

* **Explanation:** Scattered returns without a structured cleanup label or lifecycle owner cause resource leaks when error conditions occur.

### SHOULD-05: Mixed Responsibilities in Single Function

* **Original:**

```
int InitSensor(char *name) {
    /* string copies, memory allocation, register queries,
       chip validation, mode decoding, calculation */
}


```

* **Fixed:**

```
int Sensor_ReadRegisters(Reg_t *regs, size_t count);
int Sensor_ValidateID(const Reg_t *regs);
int Sensor_ConfigureMode(int mode_val);
int InitSensor(const char *name);


```

* **Explanation:** Combining dynamic memory allocation, register I/O, device validation, and formatting violates the Single Responsibility Principle.

### SHOULD-06: Inconsistent Naming Conventions

* **Original:**

```
typedef struct { int Addr; int Val; } Reg_t;
int cnt;
Reg_t *Regs;


```

* **Fixed:**

```
typedef struct {
    int address;
    int value;
} SensorRegister;

static SensorRegister *g_sensor_registers;


```

* **Explanation:** Mixing PascalCase fields (`Addr`, `Val`) with lower abbreviated globals (`cnt`) and uppercase pointers (`Regs`) degrades readability.

### SHOULD-07: Unused Local Variable

* **Original:**

```
int i, ret, unused;


```

* **Fixed:**

```
int i, ret;


```

* **Explanation:** Declaring `unused` on the stack without reading or writing it adds noise and triggers compiler diagnostic warnings.

### SHOULD-08: Code and Comment Discrepancy

* **Original:**

```
if (t < lim) printf("t is small\n");   // increment t by one


```

* **Fixed:**

```
if (t < lim) {
    printf("t is small\n");
    t++;
}


```

* **Explanation:** The comment indicates an increment was intended, but no increment operation exists in the executable logic.


## 3. NIT (Minor Polish, Warnings & Coding Standards)

### NIT-01: Missing Braces Around Control Statements

* **Original:**

```
if (cnt > 1) ret = 0;
else if (cnt == 1) ret = -1;


```

* **Fixed:**

```
if (cnt > 1) {
    ret = 0;
} else if (cnt == 1) {
    ret = -1;
}


```

* **Explanation:** Omitting braces around control statements increases the likelihood of accidental nesting errors during future maintenance.

### NIT-02: Non-Const Pointer Parameter

* **Original:**

```
int InitSensor(char *name)


```

* **Fixed:**

```
int InitSensor(const char *name)


```

* **Explanation:** The input string is read-only; applying `const` prevents inadvertent modifications and clarifies parameter intent.

### NIT-03: Missing Explicit `void` in `main()`

* **Original:**

```
int main()


```

* **Fixed:**

```
int main(void)


```

* **Explanation:** In C, an empty parameter list `()` specifies an unstated number of arguments; `(void)` explicitly indicates that `main` accepts no arguments.

### NIT-04: Inconsistent Spacing in Loop Header

* **Original:**

```
for(i=0;i<=REG_CNT;i++) {


```

* **Fixed:**

```
for (i = 0; i < REG_CNT; i++) {


```

* **Explanation:** Omitting spaces around operators and after semicolons reduces code scannability and violates clean C formatting standards.

### NIT-05: Abbreviated Struct Member Names

* **Original:**

```
typedef struct { int Addr; int Val; } Reg_t;


```

* **Fixed:**

```
typedef struct {
    int address;
    int value;
} Register;


```

* **Explanation:** Truncated names like `Addr` and `Val` reduce clarity compared to full descriptive member names.

### NIT-06: Incomplete Placeholder Comment

* **Original:**

```
int ReadReg(int a) { return a * 2; }   // fake I2C read


```

* **Fixed:**

```
/**
 * @brief Test mock simulating an I2C register read. Must be replaced with real bus I/O in production.
 */
int ReadReg(int a) { return a * 2; }


```

* **Explanation:** A brief trailing comment fails to document the limitations and production requirements of the simulated mock function.