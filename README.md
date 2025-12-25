# Library Management System - OS Concepts Simulation (Phase 2)

A multithreaded library management system demonstrating key operating system concepts in C.

## 📋 Project Overview

This is a **Phase 2 implementation** of a library management system that simulates:
- **Process Management**: Parent process (Builder) spawns child processes (Library, Client)
- **Multithreading**: Multiple user threads within the Client process, multiple handler threads in the Library
- **Synchronization**: Read-Write locks (pthread_rwlock) for concurrent data access
- **IPC**: Network sockets for client-server communication
- **File Locking**: Combined mutex and flock for thread-safe and process-safe logging

## 🏗️ Architecture

### Key Components

1. **Builder** (`builder.c`) - Main entry point
   - Initializes the log file
   - Forks the Library process
   - Forks the Client process
   - Waits for completion and manages cleanup

2. **Library** (`library.c`) - Server
   - Manages two global data structures (Users and Books)
   - Listens on port 8080 for incoming client connections
   - Creates a handler thread for each incoming request
   - Uses `pthread_rwlock_t` for concurrent access to shared tables
   - Handles: Register, Lend, Return, AddBook operations

3. **Client** (`client.c`) - Client process
   - Spawns multiple user threads (one per user config file)
   - Each thread reads instructions from its config file
   - Connects to Library socket to send commands
   - Handles Sleep instructions for timing control

4. **Common Header** (`common.h`) - Shared utilities
   - Logging function with both mutex (thread-safe) and flock (process-safe)
   - Constants and includes
   - Network configuration

## 🔑 Key OS Concepts Implemented

### 1. **Process Management**
- Fork/Exec: Builder spawns Library and Client as separate processes
- Wait: Builder waits for child processes to complete
- Signals: SIGTERM used to terminate Library process

### 2. **Multithreading**
- `pthread_create()`: Create handler threads in Library, user threads in Client
- `pthread_join()`: Client waits for all user threads
- `pthread_detach()`: Library handler threads release resources automatically

### 3. **Synchronization & Concurrency (Readers-Writers Problem)**

**Read-Write Locks** protect shared data:
```c
// Multiple readers can access simultaneously
pthread_rwlock_rdlock(&user_lock);      // Reader lock
// Single writer gets exclusive access
pthread_rwlock_wrlock(&book_lock);      // Writer lock
```

**Usage Examples:**
- **Register User** → Requires write lock (modifies Users table)
- **Check User Exists** → Requires read lock (only reads Users table)
- **Lend Book** → Requires write lock on Books (state change)
- **Find Book** → Could use read lock (search only)

### 4. **File Locking (Mutex + flock)**

Two-layer protection for logging:
```c
pthread_mutex_lock(&log_mutex);  // Thread-level: Prevents racing within process
flock(fd, LOCK_EX);              // Process-level: Prevents racing between processes
```

### 5. **Socket Programming (IPC)**

Client-Server communication via TCP sockets:
- Server: `socket()` → `bind()` → `listen()` → `accept()`
- Client: `socket()` → `connect()` → `send()`/`read()`

## 📁 File Structure

```
OS_Project/
├── common.h          # Shared header with logging and constants
├── library.c         # Library server implementation
├── client.c          # Client process with user threads
├── builder.c         # Main orchestrator process
├── Makefile          # Build configuration
├── user1.txt         # User instructions (Bob's actions)
├── user2.txt         # User instructions (Alice's actions)
└── log.txt          # Generated log file (after running)
```

## 🔧 Compilation

```bash
# Build all components
make

# Clean build artifacts
make clean

# Run the simulation (builds if needed)
make run
```

Or compile manually:
```bash
gcc -Wall -Wextra -pthread -g -o builder builder.c -pthread
gcc -Wall -Wextra -pthread -g -o library library.c -pthread
gcc -Wall -Wextra -pthread -g -o client client.c -pthread
```

## 🚀 Running the Simulation

```bash
./builder
```

The simulation will:
1. Start the Library server (listening on port 8080)
2. Start the Client process (with 2 user threads)
3. Execute user instructions from `user1.txt` and `user2.txt`
4. Log all activities to `log.txt`
5. Clean up and exit

## 📊 Example User Configuration Files

### user1.txt (Bob's instructions)
```
Register Bob              # Register as a user
Sleep 1.5               # Wait 1.5 seconds
Lend Harry_Potter Bob   # Borrow Harry Potter
Sleep 2                 # Wait 2 seconds
Return Harry_Potter     # Return the book
```

### user2.txt (Alice's instructions)
```
Register Alice                        # Register as a user
Sleep 0.5                            # Wait 0.5 seconds
Lend 1984 Alice                      # Borrow 1984
Sleep 1.5                            # Wait 1.5 seconds
Return 1984                          # Return the book
Sleep 1                              # Wait 1 second
Lend To_Kill_a_Mockingbird Alice     # Borrow another book
```

## 📝 Log Output Format

```
=== Library Management System Simulation ===

[BUILDER] Simulation Started.
[LIBRARY] Library process starting...
[LIBRARY] Server listening on port 8080...
[CLIENT_PROC] Starting 2 client threads...
[CLIENT_THREAD] User thread started, processing: user1.txt
[LIBRARY_THREAD] Request received: Register Bob
[LIBRARY_THREAD] Response sent: success 2
[CLIENT_THREAD] Command: Register Bob | Response: success 2
...
[BUILDER] Simulation Finished.
```

**Log Format:** `[SOURCE] Message`

Sources include:
- `BUILDER` - Main builder process
- `LIBRARY` - Library server process
- `LIBRARY_THREAD` - Library handler thread
- `CLIENT_PROC` - Client process
- `CLIENT_THREAD` - Client user thread
- `CLIENT_ERR` - Client error messages

## 🧪 Testing Scenarios

The default configuration tests:
1. **Concurrent User Registration** - Both users register simultaneously
2. **Book Lending & Returning** - Multiple users access the same books
3. **Timing Control** - Sleep commands verify synchronization
4. **Lock Efficiency** - Read-Write locks allow concurrent reads while protecting writes

## 🔒 Synchronization in Action

### Scenario: Alice and Bob both trying to register

```
Time 1: Alice thread → Lend 1984 (acquires WRITE lock on books)
Time 2: Bob thread  → Try to Lend Harry_Potter (waits for lock)
Time 3: Alice returns book → Releases lock
Time 4: Bob acquires lock and proceeds
```

The Read-Write lock prevents:
- **Race Conditions**: Preventing simultaneous writes
- **Data Corruption**: Ensuring consistent state
- **Deadlocks**: Non-blocking read access when only reading

## 🛠️ Key Implementation Details

### Data Structures
```c
typedef struct {
    char name[50];
    int id;
} User;

typedef struct {
    char title[50];
    int available; // 1 = Yes, 0 = No
} Book;
```

### Global Tables
- `User users[MAX_USERS]` - Array of registered users
- `Book books[MAX_BOOKS]` - Array of available books
- `pthread_rwlock_t user_lock` - Protects users table
- `pthread_rwlock_t book_lock` - Protects books table

### Handler Thread Lifecycle
1. Accept incoming connection
2. Parse client request
3. Acquire appropriate lock (read or write)
4. Perform operation
5. Release lock
6. Send response
7. Close connection and exit

## 🚨 Error Handling

The system handles:
- Socket connection failures with retry logic
- Missing user files
- Non-existent books
- Duplicate user registrations
- File operation errors

## 📚 OS Concepts Demonstrated

| Concept | Implementation |
|---------|-----------------|
| Process Management | `fork()`, `execl()`, `wait()`, `kill()` |
| Multithreading | `pthread_create()`, `pthread_join()`, `pthread_detach()` |
| Synchronization | `pthread_rwlock_rdlock()`, `pthread_rwlock_wrlock()` |
| IPC (Sockets) | TCP client-server communication |
| File Locking | `flock()` for inter-process safety |
| Mutex | `pthread_mutex_lock()` for thread safety |

## 🎓 Learning Outcomes

After studying this implementation, you should understand:
- How multiple processes communicate via sockets
- How threads within a process share memory safely
- The Readers-Writers problem and its solution
- Why multiple synchronization mechanisms are needed
- How to prevent race conditions and data corruption
- Process lifecycle and inter-process communication

## ⚠️ Limitations & Future Enhancements

Current limitations:
- Fixed book list (pre-populated in library)
- Basic command parsing (whitespace-delimited)
- No user authentication beyond existence check
- Limited error messages
- Manual user configuration files

Potential enhancements:
- Dynamic book management via AdminPanel
- User password authentication
- Transaction logging
- Persistent storage (database)
- Load balancing for multiple clients
- Connection pooling
- Request queuing and priority handling

## 📞 Debugging

To see detailed execution:
1. Check `log.txt` for all process/thread activities
2. Modify `user1.txt` or `user2.txt` to test different scenarios
3. Add timing information to understand concurrency behavior
4. Use `gdb` to debug specific issues:
   ```bash
   gdb ./library
   gdb ./client
   ```

## 📖 References

- POSIX Threads: `man pthread_create`, `man pthread_rwlock_init`
- Socket Programming: `man socket`, `man connect`
- File Operations: `man flock`, `man open`
- Process Management: `man fork`, `man execl`

---

**Author Notes**: This implementation emphasizes educational clarity over production robustness. Real-world systems would include additional error handling, logging frameworks, and configuration management.
