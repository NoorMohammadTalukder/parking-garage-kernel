# Parking Garage Kernel Module

A Linux kernel module written in C that implements a
real-time parking garage management system with
scheduling capabilities.

---

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [How To Compile](#how-to-compile)
- [How To Run](#how-to-run)
- [Usage](#usage)
- [Technical Details](#technical-details)
- [Author](#author)

---

## Overview

This project demonstrates core Operating Systems concepts
by building a real Linux kernel module from scratch.
The module manages a 5-floor parking garage where:

- Cars can enter and exit dynamically
- Each floor tracks its own cars using kernel linked lists
- A background kernel thread monitors occupancy 24/7
- Live status is readable anytime via /proc/parking

---

## Features

- **/proc/parking** — Virtual file showing live garage
  status including floor-by-floor breakdown
- **/proc/parking_input** — Write interface for adding
  and removing cars from user space
- **Multiple Car Types** — Compact (1 space), SUV
  (2 spaces), Truck (3 spaces)
- **Per Floor Tracking** — Each floor maintains its
  own linked list of parked cars
- **kthread Background Manager** — Kernel thread runs
  every 5 seconds checking occupancy and issuing
  warnings when capacity is low
- **Mutex Synchronization** — All shared data protected
  with DEFINE_MUTEX to prevent race conditions
- **Producer Program** — User space C program to add
  cars to the garage
- **Consumer Program** — User space C program to remove
  cars from the garage

---

## Project Structure
```
parking-garage-kernel/
├── src/
│   └── parking.c        ← Main kernel module
├── userspace/
│   ├── producer.c       ← Add cars to garage
│   └── consumer.c       ← Remove cars from garage
├── Makefile             ← Build file
└── README.md            ← This file
```

---

## Requirements

- Linux Ubuntu 22.04 or 24.04
- Kernel headers matching running kernel
- GCC compiler
- make build tool
- Root access (sudo)

Install requirements:
```bash
apt install build-essential linux-headers-$(uname -r) gcc make
```

---

## How To Compile

### Compile kernel module:
```bash
make
```

### Compile user space programs:
```bash
gcc -o userspace/producer userspace/producer.c
gcc -o userspace/consumer userspace/consumer.c
```

---

## How To Run

### Step 1 — Load the module
```bash
insmod src/parking.ko
```

### Step 2 — Check it loaded
```bash
dmesg | tail -5
```

You should see:
```
Parking Garage: module loaded
Parking Manager: thread started
Parking Manager: Cars=0 Available=50
```

### Step 3 — View garage status
```bash
cat /proc/parking
```

### Step 4 — Add cars
```bash
./userspace/producer compact 3
./userspace/producer suv 5
./userspace/producer truck 1
```

### Step 5 — View updated status
```bash
cat /proc/parking
```

### Step 6 — Remove a car
```bash
./userspace/consumer 1
```

### Step 7 — Unload module when done
```bash
rmmod parking
```

---

## Usage

### Producer (Add Cars)
```bash
./producer <type> <floor>

Types:  compact, suv, truck
Floor:  1 to 5

Examples:
./producer compact 3    # Add compact car to floor 3
./producer suv 5        # Add SUV to floor 5
./producer truck 1      # Add truck to floor 1
```

### Consumer (Remove Cars)
```bash
./consumer <car_id>

Examples:
./consumer 1    # Remove car with ID 1
./consumer 3    # Remove car with ID 3
```

### Monitor Live Status
```bash
# One time check
cat /proc/parking

# Live auto refresh every second
watch -n1 cat /proc/parking
```

### Sample Output
```
Parking Garage Status
---------------------
Total Floors : 5
Total Spaces : 50
Cars Parked  : 3
Available    : 44
Status       : OK

Floor Details:
  Floor 5: [Car#2 suv]
  Floor 4: [ Empty ]
  Floor 3: [Car#1 compact]
  Floor 2: [ Empty ]
  Floor 1: [Car#3 truck]
```

---

## Technical Details

### Kernel Concepts Used

| Concept | Implementation |
|---------|---------------|
| Kernel Module | Loadable module with init and exit |
| /proc filesystem | Custom virtual files for status and input |
| Linked List | list_head for per-floor car queues |
| Mutex | DEFINE_MUTEX protecting shared data |
| kthread | Background manager checking every 5 seconds |
| kmalloc/kfree | Dynamic kernel memory management |
| copy_from_user | Safe user to kernel data transfer |

### Car Types and Space Usage

| Type    | Spaces Used |
|---------|-------------|
| Compact | 1 space     |
| SUV     | 2 spaces    |
| Truck   | 3 spaces    |

### Garage Specifications

| Property      | Value    |
|---------------|----------|
| Total Floors  | 5        |
| Total Spaces  | 50       |
| Warning Level | 10 spaces remaining |

### Key Functions
```c
enter_garage(type, floor)  // Add car to garage
exit_garage(car_id)        // Remove car from garage
garage_manager()           // kthread background monitor
parking_show()             // Generate /proc/parking output
parking_write()            // Handle /proc/parking_input
```

---

## Author

**Noor Mohammad Talukder**
Ph.D. Student, Computer Science
Florida State University

- Email: nt25e@fsu.edu
- GitHub: github.com/NoorMohammadTalukder
- LinkedIn: linkedin.com/in/noormohammadtalukder
