/* tce_tiny.cpp
 *
 * Stripped down version of my larger tail-call-elimination.cpp demo/benchmark
 * program, using only the scheme I chose to use in the VM.
 *
 * I created a virtual machine for a graph-based simulation language for program
 * constraint analysis.  To get reasonable speed from it, I wanted to avoid
 * using inline assembler (as that would make me the only maintainer at the
 * company I was working at), so I looked into things like tail-call elimination
 * which was pretty effective.
 *
 * Since I left the company, I couldn't take the code with me, so I thought I'd
 * build this refresher so I could recall all the important steps should I ever
 * need to use this optimization technique in another project.
 *
 * MCM 20190917 - added documentation so people can get a better feel for it,
 *      PGM_* variables to configure the test program so the slow version can
 *      run without crashing the stack.
 * MCM 20160131 - original version
 */
#include <stdio.h>
#include <chrono>

//==================== VIRTUAL MACHINE DETAILS ==============================

// To take advantage of tail-call elimination, all the functions need to have
// the same signature:
typedef void (*VOP)(void);

// My VM executed small programs, so a 'memory space' of 100K items was much
// larger than I actually needed.  I can always increase the space, if needed.
VOP OPARRAY[100005];

// Registers for the VM:
int IP = 0,                                             // Instruction pointer
    SP = (sizeof(OPARRAY) / sizeof(OPARRAY[1])) - 1;    // Stack pointer

int FLAGS = 0;
enum flags {
    ZR = 1,                     // If last operation was 0
    CY = 2,                     // Last operation generated carry/borrow
};

//==================== VIRTUAL MACHINE OPCODE HANDLERS ======================

// NOTE: The number of instructions executed by the trivial test program is
// the product of these two values.  If you want to run the slow version, the
// stack will overflow with the values shown.  Using 100 and 10 below is what
// I used for the "RESULTS" section in the README.md.
#define PGM_SIZE    10000
#define PGM_LOOPS   100000

// I currently don't care to implement a bunch of actual opcodes, so I'm just
// throwing some ugly bits together as an example.
int f1, f2, f3, f4;

void func1(void) {
    ++f1;
    OPARRAY[++IP]();
}

void func2(void) {
    ++f2;
    OPARRAY[++IP]();
}

void func3(void) {
    ++f3;
    OPARRAY[++IP]();
}

// This opcode will return (ending program execution) once it's executed
// over 100K times.  Otherwise, it goes back to the first instruction.
void func4(void) {
    ++f4;
    if (f4>PGM_LOOPS)
        return;
    OPARRAY[IP=0]();
}

int main(int, char**) {
    using namespace std::chrono;

    // Fill the first 10K slots of memory with func1(), func2(), and func3()
    // opcodes, finishing off with a func4() at the end.  Thus it turns into
    // a program that increments the f1, f2, and f3 variables a number of
    // times, and f4 at the end of the program, making it loop 100K times.
    for (int i=0; i<PGM_SIZE; ++i) {
        if (i%17==0) { OPARRAY[i]=&func3; }
        else if (i%13) { OPARRAY[i]=&func2; }
        else {OPARRAY[i]=&func1; }
    }
    OPARRAY[PGM_SIZE]=&func4;
    puts("BOOM!");
    IP=0;
    steady_clock::time_point start = std::chrono::steady_clock::now();
    OPARRAY[IP]();
    steady_clock::time_point end = std::chrono::steady_clock::now();
    duration<double> span = duration_cast<duration<double>>(end-start);
    printf("KERBLAM! IP:%u, f1:%u, f2:%u, f3:%u, f4:%u\n%f seconds\n",
            IP, f1, f2, f3, f4, span.count());

    int num_instructions_executed = f1+f2+f3+f4;
    double ins_per_sec = num_instructions_executed / span.count();
    printf("instructions executed: %u, instructions/second: %f\n", num_instructions_executed, ins_per_sec);
}
