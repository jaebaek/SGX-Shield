------------------------
Purpose of SampleEnclave
------------------------
The project demonstrates several fundamental usages of Intel(R) Software Guard 
Extensions (SGX) SDK:
- Initializing and destroying an enclave
- Creating ECALLs or OCALLs
- Calling trusted libraries inside the enclave

------------------------------------
How to Build/Execute the Sample Code
------------------------------------
1. Install Intel(R) SGX SDK for Linux* OS
2. Build the project with the prepared Makefile:
    a. Hardware Mode, Debug build:
        $ make SGX_MODE=HW SGX_DEBUG=1
    b. Hardware Mode, Pre-release build:
        $ make SGX_MODE=HW SGX_PRERELEASE=1
    c. Hardware Mode, Release build:
        $ make SGX_MODE=HW
    d. Simulation Mode, Debug build:
        $ make SGX_DEBUG=1
    e. Simulation Mode, Pre-release build:
        $ make SGX_PRERELEASE=1
    f. Simulation Mode, Release build:
        $ make
3. Execute the binary directly:
    $ ./app


------------------------------------
Jaebaek: Build/Execute attack demos
------------------------------------
1. Build an attack program using (source to sgx aslr)/linux/program/attack/sample*.c
2. Build this project with the Makefile:
    a. Enable debugging message (default: OFF)
        $ make DEBUG=ON
    b. Enable random allocation (default: ON)
        $ make RAND=OFF
    c. Enable an attack
        $ make TECH=<attack> ATTACKER=<power>

        <attack> can be 0 (RET_TO_FUNC), 1 (ROP), 2 (ROP_EEXIT)
        <power> can be 0 (KERNEL), 1 (HOST_PROG), 2 (REMOTE), 3 (MEM_LEAK)

        Note that the code injection do not need any option
3. Execute the binary directly:
    $ ./app
