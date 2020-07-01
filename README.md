# tp-2019-1c-creativOS

## Context
The project was part of the Operating Systems subject and its objective was to develop a solution that allows the simulation of certain aspects of a distributed database, that is, where the different system resources can carry out a workload running from different points.
Its parts were filesystem, memories and kernel


## Goals
* Acquire practical concepts of the use of the different programming tools and interfaces (APIs) provided by operating systems.
* Understand aspects of the design of an operating system.
* Affirm various theoretical concepts of the subject through the practical implementation of some of them.
* Get familiar with system programming techniques, such as using makefiles, configuration files, and log files.
* Get to know Linux operations in detail by using a relatively low-level programming language such as C.
  * Threads
  * Processes


## Full requirements:
https://docs.google.com/document/d/1QlzXwpSvI5ua2lbO8pF6ZgjlgMndFlwzlAci7qhZmqE/edit#

## Final tests:
https://docs.google.com/document/d/1m_V2AXpfo8SpeOr330Rwj3uKIe-GHJ3VNfO38FNNc6Q/edit#

## Previous steps to import the project on Eclipse:
1. If you previously had installed on your sistem the commons library, go to the commons directory and run `sudo make uninstall`

## Steps to import the proyects on Eclipse:
1. Clone the repository
2. Run `cd commons` (of the repository) and then run `sudo make install`
3. In Eclipse go to `New -> Makefile Project with Existing Code -> Import Kernel and choose Linux GCC`
4. In Eclipse go to `New -> Makefile Project with Existing Code -> Import PoolMemorias and choose Linux GCC`
5. In Eclipse go to `New -> Makefile Project with Existing Code -> Import LFS and choose Linux GCC`

## Steps to modify functions of the commons library:
1. In Eclipse go to `New -> Makefile Project with Existing Code -> Import commons and choose Linux GCC`
2. Add the new *file.c* and *file.h* or edit the existing ones
3. Open the terminal, go to the directory `commons` and run:
    1. `sudo make uninstall`
    2. `sudo make install`
