# Test Cases for Library Management System

## Testing the Readers-Writers Lock Implementation

This directory contains test scenarios to demonstrate the key OS concepts.

## Test Case 1: Basic Operation (DEFAULT)

**Files**: `user1.txt`, `user2.txt`
**What it tests**: 
- Basic register, lend, and return operations
- Concurrent user registration
- Interleaved I/O across multiple threads

**Expected behavior**:
- Both users register successfully with unique IDs
- Books are lended and returned without corruption
- Log entries don't show jumbled text (file locking works)

---

## Test Case 2: Concurrent Book Lending

Create `user3.txt`:
```
Register Carol
Sleep 0.2
Lend Harry_Potter Carol
Sleep 3
Return Harry_Potter
```

Create `user4.txt`:
```
Register Dave
Sleep 0.3
Lend Harry_Potter Dave
Sleep 2
Return Harry_Potter
```

Run with:
```bash
# Modify builder.c line 42 to:
execl("./client", "client", "user3.txt", "user4.txt", (char *)NULL);
```

**What it tests**:
- Write lock competition on Books table
- One user must wait while another holds the book
- Race condition prevention

**Expected behavior**:
- Carol lends Harry_Potter successfully (0.2s)
- Dave waits for lock, then sees "book not available" (0.3s > 0.2s)
- Carol returns the book
- Dave can now lend it

---

## Test Case 3: High Concurrency (Many Users)

Create multiple user files:

`user_fast.txt`:
```
Register User_Fast
Lend 1984 User_Fast
Return 1984
```

`user_slow.txt`:
```
Register User_Slow
Sleep 0.5
Lend To_Kill_a_Mockingbird User_Slow
```

Run simultaneously with 5+ threads to stress test locks.

**What it tests**:
- Multiple handler threads in library
- Coherence under high concurrency
- Deadlock prevention

---

## Test Case 4: File Locking Safety

Manually inspect `log.txt` during execution to verify:
- No partial log entries (flock works)
- No garbled text from simultaneous writes
- Chronological consistency

**To test this**:
1. Run `./builder` and quickly check `log.txt` mid-execution
2. Verify all entries are complete (ends with \n)
3. Verify no mixed content from different log sources

---

## Test Case 5: Error Handling

Create `user_error.txt`:
```
Register ErrorUser
Lend NonExistentBook ErrorUser
Return NonExistentBook
Lend Harry_Potter NonExistentUser
```

**What it tests**:
- Error cases (non-existent books/users)
- Server stability under failures
- Graceful error messages

**Expected behavior**:
- Registration succeeds
- Lend non-existent book fails gracefully
- Lend with non-existent user fails gracefully
- Server continues handling requests

---

## Test Case 6: Race Condition Detection (Readers-Writers)

Create `reader_thread.txt`:
```
Register Reader
Sleep 0.1
Sleep 0.1
Sleep 0.1
```

Create `writer_thread.txt`:
```
Register Writer
Sleep 0.05
AddBook NewBook
Sleep 0.1
```

**What it tests**:
- Read locks (Sleep = no-op command logs reads)
- Write locks (Register/AddBook = writes)
- Proper lock sequencing

**Expected behavior**:
- Multiple sleeps can happen in parallel (read lock friendly)
- AddBook is exclusive (write lock)
- No data corruption

---

## How to Run Custom Test Cases

1. Create your user configuration files in the project directory
2. Modify `builder.c` line 42 to reference your files:
   ```c
   execl("./client", "client", "user_a.txt", "user_b.txt", "user_c.txt", (char *)NULL);
   ```
3. Run `make run`
4. Inspect `log.txt` for results

---

## Analyzing Results

### Verify Thread Safety
Look for:
- ✅ All log entries properly formatted with process/thread name
- ✅ No missing or corrupted lines
- ❌ No garbled text mixing entries

### Verify Synchronization
Look for:
- ✅ User IDs are unique (ID sequencing)
- ✅ Books become unavailable when lended
- ✅ Books become available when returned
- ✅ Proper error messages for edge cases

### Verify IPC
Look for:
- ✅ Client successfully connects to library
- ✅ Requests are received by library handler threads
- ✅ Responses are returned to clients
- ✅ No connection timeouts or failures

---

## Common Issues & Troubleshooting

### Issue: "Connection Failed" in log
**Cause**: Library server hasn't started yet
**Fix**: Builder adds `sleep(2)` before forking client. Increase if needed.

### Issue: Garbled log output
**Cause**: File locking not working properly
**Fix**: Verify `flock()` is being called. Check file permissions.

### Issue: Users with same ID
**Cause**: Race condition in user registration
**Fix**: Verify write lock is acquired before incrementing user_count

### Issue: Book lend succeeds for unavailable book
**Cause**: Lock not held during availability check + modification
**Fix**: Verify lock is acquired before ANY access to books array

---

## Extending the Project

### Add more commands:
- `ListBooks` - Display all available books
- `ListUsers` - Show registered users
- `WaitForBook <title>` - Queue for unavailable book

### Add persistence:
- Save user/book state to database
- Restore state on restart

### Add authentication:
- Password verification
- Access control levels

### Add performance monitoring:
- Track lock wait times
- Measure throughput
- Profile thread creation overhead

---

## Key Performance Metrics

Run the default test and measure:

```bash
time ./builder
```

Expected output:
- Total execution time: ~5-10 seconds
- Library startup: <1 second
- Client execution: ~5 seconds (dominated by Sleep commands)
- Cleanup: <1 second

Log size:
- Expect 30-50 lines per default test
- Each user action generates 2+ log entries (request + response)

---

## References for Further Study

1. **POSIX Threads**: Study `pthread_rwlock_t` semantics
2. **Reader-Writer Problem**: Classic OS synchronization challenge
3. **Socket IPC**: Compare with pipes, message queues, shared memory
4. **File Locking**: Understand `flock()` vs `fcntl()` semantics
5. **Process vs Thread**: Trade-offs in memory/context switching

