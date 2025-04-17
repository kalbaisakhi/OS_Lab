# **CS39002 Operating Systems Laboratory**  
**Spring 2025**  

## **Lab Assignment: 5**  
**Date:** 05–February–2025  

---

## **IPC using Shared Memory**  

In this assignment, one **leader process** \(L\) and **n follower processes** \(F_1, F_2, \dots, F_n\) cooperatively share a memory segment **M**. The leader **L** first creates the segment **M**. The individual integer-valued cells of **M** store the following items:

- **M[0]** always stores **n** (the number of followers).  
- **M[1]** stores the number of followers that have joined so far.  
- **M[2]** stores the turn of the process to access the shared memory (**0 for leader L, i for Follower Fᵢ**).  
- **M[3]** is the cell where the leader **L** writes.  
- **M[4], M[5], ..., M[3+n]** are the cells where the followers \(F_1, F_2, \dots, F_n\) respectively write.  

**Assume that \( n \leq 100 \).** Based on this limit, determine the maximum size of **M**.  

After creating **M**, the leader **L** waits until all **n** followers join. **Race condition** is possible during the joining of the followers, because they access and modify **M[1]**. In reality, a situation where race conditions may leave **M[1]** in an inconsistent state is very rare. **Ignore such rare occurrences** (i.e., use no preventive measures for **M[1]**). If it is a problem on your platform, use **sleep()** judiciously.  

After all followers have joined, no race condition is possible. This is ensured by alternating the turns of the leader and the followers, as indicated by **M[2]**. Each process performs a **busy wait** until its turn comes. When it is done with its turn, the process sets **M[2]** to break the busy wait of the next process.  

The leader **L** initializes **M[2]** to **0** so that it is the leader's turn first.  

### **Termination Condition**  
The leader **L** tracks the sums it has computed so far using a **hash table** (implemented manually, not a built-in one). This array is **not shared** but is stored in **L's local memory**.  

- As soon as a sum is **duplicated (for the first time)**, **L** sets **M[2] = –1** instead of **1**.  
- Each follower **Fᵢ** follows this logic:  
  - When **M[2] = i**, the follower writes its value.  
  - When **M[2] = –i**, the follower **terminates**, but before exiting:  
    - It sets **M[2]** to **–(i + 1)** if \( i < n \), or **0** if \( i = n \), to wake up the next process.  
- When **L** wakes up for the last time, it realizes all followers are gone, removes **M**, and terminates.  

---

## **Execution Details**  

### **Leader Process \( L \)**  

1. **L writes a random integer** in the range **1–99** to **M[3]**.  
2. **L sets M[2] to 1** to wake up **F₁** and then enters a **busy wait** until **M[2] becomes 0** again.  
3. Each follower writes a **random integer** in the range **1–9** to its respective cell and updates **M[2]** to indicate the next process's turn.  
4. After all followers have written their values, **L wakes up**, computes the **sum**, and prints it.  

---

## **Implementation Details**  

### **Leader Code \( leader.c(pp) \)**  
- Write **leader.c(pp)** to implement **L**.  
- **n** may be supplied as a **command-line argument** (default **n = 10**).  
- **L** first **creates the shared memory segment \(M\)** and initializes **M[0], M[1], and M[2]**.  
- **L** waits until **M[1]** becomes **n** (all followers have joined).  
- **L** then enters the sum-computation loop.  
- Upon detecting a duplicate sum, **L** initiates the termination sequence and finally **removes M** and exits.  

---

### **Follower Code \( follower.c(pp) \)**  
- Write **follower.c(pp)** to implement **followers**.  
- Followers **run independently** from separate shell executions.  
- **A command-line argument specifies the number of followers \( n_f \) to create** (default **n_f = 1**).  
- Each follower:  
  - Gets the ID of **M** (it must **not** create it).  
  - Increments **M[1]** and determines its **follower number \( i \)**.  
  - Participates in the **summand-generation loop**.  
  - Completes the **termination sequence** and exits.  
- **Followers are not forked by the leader**. They are created by independent executions of **follower.c**.  

---

## **Sample Execution & Output**  

### **Terminal 1: Leader Execution**  
```bash
$ ./leader 15
Wait for the moment.
```

### **Terminal 2: Single Follower Execution**  
```bash
$ ./follower
follower 1 joins
```

### **Terminal 3: Multiple Followers Execution**  
```bash
$ ./follower 10
follower 2 joins
follower 3 joins
follower 4 joins
follower 5 joins
follower 6 joins
follower 7 joins
follower 8 joins
follower 9 joins
follower 10 joins
follower 11 joins
```

### **Terminal 4: Additional Followers**  
```bash
$ ./follower 8
follower 12 joins
follower 13 joins
follower error: 15 followers have already joined
follower 14 joins
follower 15 joins
follower error: 15 followers have already joined
follower error: 15 followers have already joined
follower error: 15 followers have already joined
```

### **Final Output in Terminal 1 (Leader)**  
```bash
88 + 4 + 9 + 4 + 8 + 8 + 4 + 3 + 9 + 7 + 5 + 3 + 4 + 6 + 1 + 6 = 169
25 + 9 + 7 + 4 + 1 + 8 + 4 + 7 + 8 + 5 + 4 + 5 + 5 + 8 + 4 + 9 = 113
34 + 9 + 9 + 4 + 5 + 1 + 6 + 4 + 5 + 8 + 6 + 4 + 5 + 8 + 6 + 9 = 123
```

### **Follower Termination Messages**  

#### **Terminal 2**  
```bash
follower 1 leaves
```

#### **Terminal 3**  
```bash
follower 2 leaves
follower 3 leaves
follower 4 leaves
follower 5 leaves
follower 6 leaves
follower 7 leaves
follower 8 leaves
follower 9 leaves
follower 10 leaves
follower 11 leaves
```

#### **Terminal 4**  
```bash
follower 12 leaves
follower 13 leaves
follower 14 leaves
follower 15 leaves
```

---

