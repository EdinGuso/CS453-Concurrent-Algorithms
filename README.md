# CS453-Concurrent Algorithms

This repo includes the project I worked on for the CS453: Concurrent Algorithms course at EPFL during Fall 2022 semester.

## Project Summary:

Project goal was to implement a Softrware Transactional Memory (STM). Maximum grade was awarded to correct implementations that do not have any memory leaks and achieve a >x2.5 speedup compared to the reference implementation. My implementation satisfied all the conditions with x2.918 speedup, receiving the maximum grade.

## Repository Structure:
* [My Implementation](https://github.com/EdinGuso/CS453-Concurrent-Algorithms/tree/main/335740): I implemented a STM using the Transactional Locking (TL2) algorithm.
  * `tm.c`: Implements all the required STM functions.
  * `transaction.h` & `transaction.c`: Transaction struct and its functionalities.
  * `shared-lock.h` & `shared-lock.c`: Lock used to control the access to shared memory region.
  * `versioned-spinlock.h` & `versioned-spinlock.c`: Versioned spinlock that performs bounded passive back-off on acquisition.
  * `read-set.h` & `read-set.c`: Read set that stores the addresses of read operations to be validated at commit time.
  * `write-set.h` & `write-set.c`: Write set that stores the addresses and values of write operations to be validated and commited at commit time.
* [Reference Implementation](https://github.com/EdinGuso/CS453-Concurrent-Algorithms/tree/main/reference): This is a naive implementation using a global lock to prevent concurrent access on the shared memory. The speedup of my implementation was computed with respect to this one.
* [Grading](https://github.com/EdinGuso/CS453-Concurrent-Algorithms/tree/main/grading): This is the program used to run my STM implementation and the reference implementation, measuring execution speed and computing the speedup.
* [Headers](https://github.com/EdinGuso/CS453-Concurrent-Algorithms/tree/main/include): Header files describing the signatures of STM library functions.

## Running the Project:

In order to compile the program and test the implementation: clone this repository, move into the grading folder and run the following command: 

`you@your-pc:/path_to_repository/grading$ make build-libs run`

**Note**: The speedup achieved highly differs depending on the machine that the test is run on. The quoted speedup (x2.918) was achieved on the following system specs:
* **CPU**: 2 (dual-socket) Intel(R) Xeon(R) 10-core CPU E5-2680 v2 at 2.80GHz (×2 hyperthreading ⇒ 40 virtual cores)
* **RAM**: 256GB
* **OS**: GNU/Linux (distribution: Ubuntu 18.04 LTS)
