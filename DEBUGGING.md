# Debugging Guide for Library Management System

## 🔍 Overview

This guide shows you how to debug the multithreaded library management system. We'll cover techniques for identifying issues, tracking execution, and understanding behavior.

---

## 1. Compile-Time Debugging

### Enable Debug Symbols
Add `-g` flag to Makefile (already present):
```bash
gcc -Wall -Wextra -pthread -g -o library library.c -pthread
```

### Strict Compilation Checks
```bash
gcc -Wall -Wextra -Werror -g -pthread library.c
```

### Check for Warnings
```bash
make 2>&1 | grep -i warning
```

---

## 2. Runtime Debugging with GDB

### Basic GDB Debugging

**Start GDB with the program:**
```bash
gdb ./library
```

**Common GDB Commands:**
```
(gdb) break main              # Set breakpoint at main
(gdb) run                     # Start program
(gdb) continue               # Continue execution
(gdb) next                   # Next line
(gdb) step                   # Step into function
(gdb) print variable_name    # Print variable value
(gdb) info threads          # List all threads
(gdb) thread 2              # Switch to thread 2
(gdb) backtrace             # Show call stack
(gdb) quit                  # Exit GDB
```

### Debug a Specific Function

**Stop at client_handler function:**
```bash
gdb ./library
(gdb) break client_handler
(gdb) run
(gdb) info args            # See function arguments
(gdb) print sock           # Print socket descriptor
```

### Debug with Arguments

**Debug builder with logging:**
```bash
gdb ./builder
(gdb) run
(gdb) backtrace            # See where it stopped
```

---

## 3. Log File Analysis

### View Logs in Real-Time
```bash
# Terminal 1: Run simulation
./builder &

# Terminal 2: Watch logs grow
tail -f log.txt
```

### Parse Specific Entries
```bash
# Count messages by source
grep -c "LIBRARY_THREAD" log.txt
grep -c "CLIENT_THREAD" log.txt

# See only errors
grep "failure\|error\|Failed" log.txt

# See connection events
grep "Connected\|closed" log.txt

# See timing (sleep) events
grep "Sleeping" log.txt
```

### Verify Synchronization
```bash
# Check user registration sequence
grep "Request received: Register" log.txt
grep "Response sent.*success" log.txt

# Verify book operations
grep "Lend\|Return" log.txt
```

---

## 4. Testing Individual Components

### Test Library Server Only
```bash
# Terminal 1: Start library
./library &
LIBRARY_PID=$!

# Terminal 2: Manual client connection
nc localhost 8080
# Type: Register Alice
# Should see response in Terminal 1's log

# Terminal 1: Stop library
kill $LIBRARY_PID
```

### Test Client Threads
Create a simple test config:
```bash
cat > test_user.txt << 'EOF'
Register TestUser
Sleep 0.5
Lend Harry_Potter TestUser
EOF

# Run client manually (but library must be running)
./library &
./client test_user.txt
```

### Test Socket Communication
```bash
# Check if port is listening
netstat -tlnp | grep 8080
lsof -i :8080

# Manual socket test
telnet localhost 8080
# Type commands and see responses
```

---

## 5. Debugging with Strace

### Trace System Calls

**Trace library process:**
```bash
strace -f -e trace=network,open,read,write -s 200 ./library
```

**Watch file operations:**
```bash
strace -f -e trace=open,read,write,flock ./builder
```

**See process forking:**
```bash
strace -f -e trace=process ./builder
```

**Output to file:**
```bash
strace -o strace.log -f ./builder
less strace.log
```

---

## 6. Debugging Specific Issues

### Issue: "Connection Failed"
```bash
# Check if library is running
ps aux | grep library

# Check if port is bound
netstat -tlnp | grep 8080

# Check firewall
sudo ufw status

# Check in log
grep "Connection" log.txt
grep "failed\|error" log.txt
```

### Issue: Race Conditions
```bash
# Run multiple times to see if behavior is consistent
for i in {1..5}; do
    echo "=== Run $i ===" >> race_test.log
    timeout 10 ./builder >> race_test.log 2>&1
    sleep 2
done

# Compare logs
diff race_test.log log.txt
```

### Issue: Memory Leaks
```bash
# Use valgrind (if installed)
valgrind --leak-check=full --show-leak-kinds=all ./library
valgrind --leak-check=full ./builder
```

### Issue: Deadlocks
```bash
# Enable timeout to detect hanging
timeout 5 ./builder
echo "Exit code: $?"  # 124 means timeout (deadlock suspected)

# Check with gdb
gdb ./builder
(gdb) run
# Press Ctrl+C after timeout
(gdb) info threads
(gdb) thread apply all backtrace
```

---

## 7. Performance Debugging

### Measure Execution Time
```bash
# Time the whole simulation
time ./builder

# Time just compilation
time make clean && make
```

### Check Thread Count
```bash
# During execution
ps -eLf | grep -E "library|client"

# In another terminal
watch -n 1 'ps -eLf | grep -E "library|client" | wc -l'
```

### Monitor System Resources
```bash
# Watch in real-time
top -H
# Press P to sort by CPU
# Press M to sort by memory

# Or use htop
htop -H
```

---

## 8. Debugging Lock Issues

### Verify Lock Acquisition

**Add debug logs in library.c:**
```c
// Before lock
write_log("DEBUG", "About to acquire write lock");
pthread_rwlock_wrlock(&user_lock);
write_log("DEBUG", "Write lock acquired");

// After operation
write_log("DEBUG", "Releasing write lock");
pthread_rwlock_unlock(&user_lock);
```

### Detect Lock Contention
```bash
# Search for lock wait patterns in log
grep -n "About to acquire" log.txt
grep -n "acquired" log.txt

# Calculate delays
# Timestamps would help (enhancement for future)
```

### Test with Stress
```bash
# Create many user files
for i in {1..5}; do
    cat > user$i.txt << 'EOF'
Register User$i
Sleep 0.1
Lend Harry_Potter User$i
Sleep 0.1
Return Harry_Potter
EOF
done

# Modify builder.c to use all 5 files
# execl("./client", "client", "user1.txt", "user2.txt", "user3.txt", 
#       "user4.txt", "user5.txt", NULL);

# Test stress
timeout 20 ./builder
grep "failure\|error" log.txt
```

---

## 9. Debugging Common Errors

### Compilation Errors

**Buffer overflow warning:**
```
warning: '%s' directive output may be truncated
```
✅ Already fixed with width specifiers (%49s)

**Undefined reference errors:**
```
undefined reference to `pthread_create'
```
Fix: Add `-pthread` flag to gcc

**Implicit function declaration:**
```
warning: implicit declaration of function 'socket'
```
Fix: Check #includes in common.h

### Runtime Errors

**Segmentation fault (SIGSEGV):**
```bash
# Use gdb to find the line
gdb ./library
(gdb) run
# Wait for crash
(gdb) backtrace
# Shows exact line that crashed
```

**Undefined behavior:**
```bash
# Use AddressSanitizer
gcc -fsanitize=address -g ./library library.c
./library
# Shows memory issues
```

---

## 10. Quick Debugging Checklist

### Before Running
- [ ] `make clean && make` - Clean rebuild
- [ ] Check for compilation warnings
- [ ] Verify books.conf exists
- [ ] Verify user1.txt and user2.txt exist

### During Execution
- [ ] Open second terminal with `tail -f log.txt`
- [ ] Monitor processes: `ps aux | grep -E "library|client|builder"`
- [ ] Check port: `netstat -tlnp | grep 8080`
- [ ] Watch resources: `top -H`

### After Execution
- [ ] Review log.txt for errors
- [ ] Check "Clean shutdown completed" message
- [ ] Verify user counts and book states
- [ ] Compare with previous runs

---

## 11. Creating Minimal Test Cases

### Test Case 1: Single User, Single Book
**test_minimal.txt:**
```
Register User1
Lend Harry_Potter User1
Return Harry_Potter
```

**Run:**
```bash
./library &
./client test_minimal.txt
killall library
```

### Test Case 2: Two Users, One Book
**user_a.txt:**
```
Register Alice
Sleep 1
Lend Harry_Potter Alice
Sleep 3
Return Harry_Potter
```

**user_b.txt:**
```
Register Bob
Sleep 1.5
Lend Harry_Potter Bob
```

**Expected:** Bob should fail or wait for Alice to return

### Test Case 3: Stress Test
**Generate 10 concurrent users:**
```bash
for i in {1..10}; do
    cat > user_stress_$i.txt << 'EOF'
Register User$i
Sleep 0.1
Lend 1984 User$i
Sleep 0.1
Return 1984
EOF
done

# Modify builder.c to include all
```

---

## 12. Debugging Tools Summary

| Tool | Purpose | Command |
|------|---------|---------|
| GDB | Interactive debugger | `gdb ./library` |
| Strace | System call trace | `strace -f ./builder` |
| Valgrind | Memory debugging | `valgrind ./library` |
| ltrace | Library call trace | `ltrace ./client` |
| netstat | Network status | `netstat -tlnp` |
| lsof | Open files | `lsof -p PID` |
| top/htop | Resource monitor | `top -H` |
| tail | Log monitoring | `tail -f log.txt` |
| grep | Log analysis | `grep pattern log.txt` |
| diff | Compare outputs | `diff log1.txt log2.txt` |

---

## 13. Environment Variables for Debugging

### GDB Script File
**debug.gdb:**
```gdb
break main
break client_handler
run
info threads
continue
```

**Use it:**
```bash
gdb -x debug.gdb ./library
```

### Environment Settings
```bash
# Disable address space layout randomization (helps debugging)
export ASLR_ENABLED=0

# Set library path
export LD_LIBRARY_PATH=/usr/lib

# Increase stack size (if needed)
ulimit -s unlimited
```

---

## 14. Debugging the Debugged Code

### Add Temporary Debug Output
```c
// In library.c client_handler:
fprintf(stderr, "[DEBUG] Received: %s\n", buffer);
fprintf(stderr, "[DEBUG] Command: %s, Arg1: %s\n", command, arg1);
fprintf(stderr, "[DEBUG] User count: %d\n", user_count);
```

**Then compile and run:**
```bash
make
./builder 2>&1 | tee debug_output.txt
# See both stdout and stderr
```

### Conditional Logging
```c
#ifdef DEBUG
write_log("DEBUG", "Lock acquired");
#endif
```

**Compile with:**
```bash
gcc -DDEBUG -g -pthread library.c -o library
```

---

## 15. Advanced: Creating a Debug Harness

**debug_harness.c:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    fprintf(stderr, "Starting library...\n");
    pid_t pid = fork();
    if (pid == 0) {
        execl("./library", "library", NULL);
    }
    
    sleep(1);
    fprintf(stderr, "Starting client...\n");
    
    // Add your test here
    execl("./client", "client", "user1.txt", NULL);
    
    return 0;
}
```

---

## Summary

**Quick Start Debugging:**
```bash
# 1. Rebuild with symbols
make clean && make

# 2. Run with output capture
./builder 2>&1 | tee run.log

# 3. Analyze log
grep "error\|failure" run.log
grep "Clean shutdown" run.log

# 4. Check with GDB if needed
gdb ./library
```

**Next Time You See an Issue:**
1. Check log.txt first
2. Grep for keywords (error, failure, closed)
3. Verify socket connections with netstat
4. Use GDB to step through problematic function
5. Add temporary logging around suspicious code

---

Happy debugging! 🐛🔍
