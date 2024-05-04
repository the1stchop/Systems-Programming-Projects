# Assignment 3 directory

## Short Description
This directory contains the source code and other files for assignment 3. In queue.c, we were tasked with implementing a bounded buffer. It has two functions push and pop, which adds or removes elements form the bounded buffer. This buffer is able to support multiple concurrent producers and multiple concurrent consumers and garantees thread order, valitity, and completeness. 

In rwlock.c, we were tasked with implementing a read/write lock. It allows multiple readers to hold the lock at the same time, but only a single
writer. It contains a lock constructor and deconstructor as well as reader lock/unlock and writer lock/unlock functions. We also implemented a PRIORITY aspect to rwlock which can take three modes: READ, WRITE, and N-WAY. if there multiple read and writes are called at the same time while pro is set to READ, then read calls should be able to proceed before writers. Vice verca when prio equals WRITE. When prio is N-WAY, n readers will proceed inbetween every write if there are writes waiting for the lock. Non-starvation properties take precidence over N-WAY, meaning if there less than n reads and there is a write waiting, the write may aquire the lock.

## Design Decisions
For queue.c I decided to follow lecture 15s peudocode for a push/pop implementation using a lock and two conditional variables. Push would aquire the lock, then check if the bounded buffer is full. If yes, then wait for a pop to make space. Once that happens, proceed and push an element to the list. Then inrement the shared counter by 1. Then release the lock. For pop, it's basically the same. Aquire lock and check if buffer is empty. If so, wait for a push. Once that happened, proceed and pop an element and decrement shared counter by 1. Then release the lock.

rwlock.c was mad hard. The first thing I did was try to implement an rwlock with the pseudocode given in the "Operating Systems Principles and Practice" textbook. This implementation used one lock and two condition variable, one for read and one for write. It did pass some of the tests but had some issues. One, it had a writer bias; if the previous thread was a write, it would give it to another write thread if one was waiting. Which was fine if prio was WRITE but it was happening every time. So what I had to additionally implement was the priority stuff. For both read and write lock, I would first check what kind of prio it was (READ, WRITE, or N-WAY). Then depending on that, I would check on if the thread should wait or proceed. For N-WAY, I also had to keep track of how many reads have already occured so I added a variable to keep track of that. If prio was N-WAY and n number of reads have occured and a writer is waiting, it should let the writer through. Then after the writer did its thing, it decremented the read counter back to zero. 


## Resources
As said earlier, I used the lecture 15 pseudocode for implemening my queue.c

I also used the "Operating Systems Principles and Practice" textbook pseudocode as a base for my rwlock.c

## Build and Clean
Rules all, queue.o, and rwlock.o, which produce the queue.o and rwlock.o object files, and the rule clean, which removes all .o and binary files
