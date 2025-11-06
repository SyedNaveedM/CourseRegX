# 🧑‍💻 CourseRegX
### A Multi-User Course Registration System built in C (Client–Server Architecture)

CourseRegX is a concurrent, socket-based course registration system that enables **Administrators**, **Faculty**, and **Students** to interact with a shared academic database in real time.  
It is built entirely in **C**, leveraging **TCP sockets**, **multi-threading**, and **binary semaphores** for safe concurrent access to shared data.

---

## 🚀 Features

### 👨‍💼 Administrator
- Manage student and faculty accounts (add, view, modify, block, activate).  
- View or update system records in real time.  

### 👩‍🏫 Faculty
- Manage offered courses (add, view, update, or remove).  
- Track enrollments and manage seat capacities.  

### 👨‍🎓 Students
- Browse all available courses and enroll/drop with real-time seat tracking.  
- View enrolled courses and update passwords securely.  

---

## 🏗️ System Design Overview

- **Language:** C  
- **Architecture:** Client–Server  
- **Concurrency:** Multi-threading via `pthread`  
- **Synchronization:** Binary semaphores (`semaphore.h`)  
- **Communication:** TCP sockets with custom protocol markers (`END`, `EXPECT`, `EXIT`)  
- **Data Storage:** Persistent `.dat` files (students, faculty, courses)  
- **Atomicity:** Temporary files + rename for safe writes  

Each connected client runs on its own thread, and file-level semaphores ensure integrity of shared data even under concurrent access.

---

## ⚙️ Compilation and Execution

### Step 1: Clone the repository
```bash
git clone https://github.com/SyedNaveedM/CourseRegX.git
cd CourseRegX
````

### Step 2: Build the project

Use the provided Makefile to compile both client and server binaries:

```bash
make
```

This will generate:

* `server`
* `client`

### Step 3: Run the Server

Start the server first to listen for incoming client connections:

```bash
./server
```

### Step 4: Run the Client

Open another terminal (on the same or a different machine) and execute:

```bash
./client
```

The client will automatically connect to the server’s IP and port defined in `common.h`.

> 💡 You can run multiple clients concurrently — each will be handled by a separate thread on the server.

---

## 🧠 Technical Highlights

* **Thread-per-client model:** Ensures concurrent request handling.
* **Semaphore-based synchronization:** Prevents race conditions on shared `.dat` files.
* **Atomic file updates:** Temporary files + rename pattern guarantees safe writes.
* **Custom socket protocol:** Message boundaries handled via special markers (`END`, `EXPECT`, `EXIT`) for reliable communication.

---

## 📸 Example Workflow

1. Admin adds a new faculty and student.
2. Faculty logs in and creates new course offerings.
3. Students view available courses and enroll or drop them in real-time.
4. Admin can activate/block accounts or modify user details as needed.

---

## 🧰 Requirements

* GCC compiler
* POSIX-compliant system (Linux/Mac)
* Make utility (`make`)
