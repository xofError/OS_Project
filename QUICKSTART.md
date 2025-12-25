# Quick Start Guide

## 🚀 Getting Started (2 minutes)

### 1. Build the Project
```bash
cd /home/xof/Desktop/OS_Project
make
```

**Expected output:**
```
gcc -Wall -Wextra -pthread -g -o builder builder.c -pthread
gcc -Wall -Wextra -pthread -g -o library library.c -pthread
gcc -Wall -Wextra -pthread -g -o client client.c -pthread
```

### 2. Run the Simulation
```bash
./builder
```

**Expected output:**
```
Simulation complete. Check log.txt for details.
```

### 3. View Results
```bash
cat log.txt
```

---

## 📖 Understanding the Output

The log shows interactions in chronological order:

```
[BUILDER] Simulation Started.           ← Main process starts
[LIBRARY] Server listening on port 8080...  ← Server ready
[CLIENT_PROC] Starting 2 client threads...  ← Client spawns users
[LIBRARY_THREAD] Request received: Register Bob
[LIBRARY_THREAD] Response sent: success 2   ← Handler thread responds
[CLIENT_THREAD] Command: Register Bob | Response: success 2
```

Each entry shows:
- `[SOURCE]` - Which process/thread generated the log
- `Message` - What happened

---

## 🔧 Modifying Behavior

### Add More Users
Edit the builder.c to spawn additional user processes:
```c
// Line 42 in builder.c, change from:
execl("./client", "client", "user1.txt", "user2.txt", (char *)NULL);

// To:
execl("./client", "client", "user1.txt", "user2.txt", "user3.txt", (char *)NULL);
```

Then create `user3.txt`:
```
Register Charlie
Sleep 1
Lend 1984 Charlie
Sleep 2
Return 1984
```

Rebuild and run:
```bash
make
./builder
```

### Create Different Scenarios
Edit `user1.txt` or `user2.txt` with different commands:

```
Register Alice          # Create user
Sleep 1.5              # Wait N seconds
Lend BookTitle UserID  # Borrow book
Return BookTitle       # Return book
AddBook NewTitle       # Add new book
```

Available books (pre-loaded):
- Harry_Potter
- 1984
- To_Kill_a_Mockingbird

---

## 🧪 Common Tests

### Test 1: Verify Thread Safety
Modify `user1.txt` and `user2.txt` to both lend the same book:
```
# user1.txt
Register Alice
Lend Harry_Potter Alice

# user2.txt
Register Bob
Sleep 0.1
Lend Harry_Potter Bob
```

Result: One succeeds, one fails with "book not available" ✓

### Test 2: Verify Lock Timeout
Create `user_slow.txt`:
```
Register SlowUser
Sleep 5
Lend Harry_Potter SlowUser
```

The Library will wait up to ~5 seconds for the client to connect.

### Test 3: Check Log Integrity
Look at `log.txt` and verify:
- No partial lines (each ends with newline)
- No mixed text from different processes
- Chronological order maintained

---

## 📊 Key Observations

### You Should See:
1. ✅ Both users register with unique IDs (1 and 2)
2. ✅ Books become "not available" after lending
3. ✅ Books become "available" after returning
4. ✅ Multiple operations handled concurrently
5. ✅ No corrupted log entries

### Performance Metrics:
- Build time: ~1 second
- Execution time: ~5-10 seconds (dominated by sleep commands)
- Log entries: 30-50 lines

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| "Connection Failed" | Wait longer before client starts (see builder.c line 28) |
| "No such file" | Ensure user1.txt and user2.txt exist in current directory |
| Build fails | Install pthread: `sudo apt install libpthread-stubs0-dev` |
| Garbled log | This shouldn't happen - indicates synchronization failure |

---

## 📚 Next Steps

1. **Read the README.md** for detailed architecture
2. **Explore TESTING.md** for more test scenarios
3. **Study the source code**:
   - `library.c` - See how RW-locks work
   - `client.c` - See how threads read files
   - `common.h` - See how logging is thread-safe
4. **Modify and experiment** - Change user files, add operations
5. **Review IMPLEMENTATION_SUMMARY.md** for deep concepts

---

## 💡 Key Concepts in This Code

### Process Management
```c
fork()   - Create library and client processes
wait()   - Builder waits for children to finish
execl()  - Replace child process with new program
kill()   - Terminate library server
```

### Multithreading
```c
pthread_create()  - Spawn user thread for each file
pthread_join()    - Wait for all threads to complete
pthread_detach()  - Handler threads cleanup automatically
```

### Synchronization
```c
pthread_rwlock_rdlock()  - Multiple readers simultaneously
pthread_rwlock_wrlock()  - Single exclusive writer
pthread_mutex_lock()     - Protect file operations
flock()                  - Prevent inter-process corruption
```

### Communication
```c
socket()   - Create network endpoint
connect()  - Client connects to server
send()     - Send command to library
read()     - Receive response from library
```

---

## ⚡ One-Command Demo

Build and run in one command:
```bash
cd /home/xof/Desktop/OS_Project && make && ./builder && echo "" && tail -20 log.txt
```

This will:
1. Build all executables
2. Run the simulation
3. Show the last 20 lines of the log

---

## 🎯 Learning Outcomes

After running this project, you understand:
- How multi-process systems organize (Builder → Library + Client)
- How threads within a process share memory (global users/books tables)
- Why Read-Write locks are efficient for readers-writers pattern
- How file locking prevents corruption from multiple processes
- How sockets enable IPC between processes
- How synchronization prevents race conditions

---

## 📞 Need Help?

1. Check `log.txt` for actual execution trace
2. Look at source comments in `.c` files
3. Read README.md for architecture explanation
4. Review TESTING.md for more examples
5. Study IMPLEMENTATION_SUMMARY.md for deep concepts

**All files are well-commented and self-explanatory!**

---

Made with ❤️ for learning OS concepts in C
