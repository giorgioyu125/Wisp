# Wisp v0.1 MVP - 20-Step Roadmap

## Current State
Modules present:
Lexer, Parser (annotation), Symbol Table, Arena, Vec, File I/O, Promise stubs

## **Phase 1: Core Foundation (Steps 1-6)**

### **Step 1: Built-in Functions Registry**
- [ ] Create `builtin_registry` (hash table of function pointers)
- [ ] Implement: `register_builtin(name, fn_ptr)`
- [ ] Add builtins: `+`, `-`, `*`, `/`, `=`, `<`, `>`, `atom`, `null?`
- [ ] Test: `(* 3 4)` → 12

**Files:** `builtins.h`, `builtins.c`

---

### **Step 2: Function Application & Lambda**
- [ ] Parse lambda: `(lambda (x y) (+ x y))`
- [ ] Store in `SymTab` as `SYM_FUNCTION` (params + body)
- [ ] Implement function application
- [ ] Test: `((lambda (x) (* x x)) 5)` → 25

**Files:** Update `eval.c`

---

### **Step 3: Error Handling Infrastructure**
- [ ] Create `WispError` struct (type, message, location)
- [ ] Implement error printing with source location
- [ ] Add error types: `SYNTAX_ERROR`, `TYPE_ERROR`, `UNBOUND_SYMBOL`, `TEMPORAL_VIOLATION`
- [ ] Wrap eval in error handler

**Files:** `error.h`, `error.c`

---

### **Step 4: Basic REPL**
- [ ] Implement read-eval-print loop
- [ ] Use GNU readline for history/editing
- [ ] Print results and errors
- [ ] Test: manual interaction

**Files:** `repl.c`, update `main.c`

---

## **Phase 2: Temporal Semantics (Steps 7-12)**

### **Step 5: Temporal Contract Parser**
- [ ] Extend lexer for contract keywords: `temporal-contract`, `precedes`, `guarantees`
- [ ] Parse contract into `TemporalContract` struct
- [ ] Store in global state
- [ ] Test: Parse example contract (no enforcement yet)

**Files:** `temporal.h`, `temporal.c`

---

### **Step 6: Promise Integration**
- [ ] Implement `(read-file-async path)` → returns `SYM_PROMISE`
- [ ] Finish `promise_tracker_poll()` 
- [ ] Add `(await promise)` primitive
- [ ] Test: Read file asynchronously

**Files:** Update `symtab.c`, `builtins.c`

---

### **Step 7: Temporal Primitives**
- [ ] Implement `(eventually pred)` → poll until true
- [ ] Implement `(when-ready promise body)` → callback on completion
- [ ] Implement `(timeout ms body)` → time-bounded execution
- [ ] Test: `(when-ready (read-file-async "test.txt") (print data))`

**Files:** Update `eval.c`, `builtins.c`

---

### **Step 8: Time Tracking**
- [ ] Add global `logical_time` counter
- [ ] Add `(current-time)` primitive
- [ ] Add `(at-time t expr)` for time-stamping
- [ ] Track symbol creation time in `Symbol` struct

**Files:** Update `temporal.c`, `symtab.h`

---

### **Step 9: Temporal Contract Verification (Basic)**
- [ ] Implement `verify_precedence(contract, execution_log)`
- [ ] Log all operations with timestamps
- [ ] Check if precedences are violated
- [ ] Error on violation
- [ ] Test: Violate a `(precedes A B)` constraint

**Files:** Update `temporal.c`

---

### **Step 10: Behavioral Equivalence**
- [ ] Make all values lazy by default (wrap in thunks)
- [ ] Implement `(force value)` to realize
- [ ] Add `(delay expr)` primitive
- [ ] Test: `(define x (delay (+ 1 2)))` → doesn't compute until forced

**Files:** Update `eval.c`

---

## **Phase 3: Usability & Polish (Steps 13-20)**

### **Step 11: List Operations**
- [ ] Implement `cons`, `car`, `cdr`, `list`
- [ ] Implement `map`, `filter`, `fold`
- [ ] Store as `SYM_LIST` in symbol table
- [ ] Test: `(map (lambda (x) (* x 2)) '(1 2 3))` → (2 4 6)

**Files:** Update `builtins.c`

---

### **Step 12: I/O Primitives**
- [ ] Implement `(print ...)`, `(display ...)`, `(newline)`
- [ ] Implement `(read-line)` for REPL input
- [ ] Implement synchronous `(read-file path)`
- [ ] Test: Print and read

**Files:** Update `builtins.c`

---

### **Step 13: Standard Library**
- [ ] Create `stdlib.wisp` with common functions
- [ ] Implement: `not`, `and`, `or`, `if`, `cond`
- [ ] Auto-load stdlib on startup
- [ ] Test: `(if #t 1 2)` → 1

**Files:** `stdlib.wisp`, add prelude loading to `main.c`

---

### **Step 14: Example Programs**
- [ ] Write `examples/hello.wisp` - basic hello world
- [ ] Write `examples/fibonacci.wisp` - recursion
- [ ] Write `examples/async-file.wisp` - temporal I/O
- [ ] Write `examples/temporal-contract.wisp` - contract demo
- [ ] Ensure all run successfully

**Files:** `examples/*.wisp`

---

### **Step 15: Memory Safety Audit**
- [ ] Run valgrind on all examples
- [ ] Fix memory leaks (especially in error paths)
- [ ] Fix use-after-free bugs
- [ ] Add assertions for invariants
- [ ] Test: Clean valgrind on all examples

**Files:** Various

---

### **Step 16: Documentation**
- [ ] Write `README.md` with philosophy and quick start
- [ ] Write `docs/TUTORIAL.md` for basic usage
- [ ] Write `docs/TEMPORAL.md` explaining temporal semantics
- [ ] Write `docs/API.md` for built-in functions
- [ ] Add code examples throughout

**Files:** `README.md`, `docs/*.md`

---

### **Step 17: Build System & Packaging**
- [ ] Create proper `Makefile` with install target
- [ ] Add `-Wall -Wextra` and fix warnings
- [ ] Add debug/release builds
- [ ] Create simple test runner script
- [ ] Test on Linux and macOS

**Files:** `Makefile`, `test.sh`

---

### **Step 18: Release Preparation**
- [ ] Version bump to 0.1.0
- [ ] Write `CHANGELOG.md`
- [ ] Tag git release: `git tag v0.1.0`
- [ ] Create release notes highlighting:
  - ✨ Temporal semantics ("behavior in time")
  - ✨ Temporal contracts (declarative)
  - ✨ Async I/O with promises
  - ⚠️ Known limitations
- [ ] Publish to GitHub

**Files:** `CHANGELOG.md`, git tag


**Total: 5-8 weeks to v0.1 so 2 weeks for each phase at max**
