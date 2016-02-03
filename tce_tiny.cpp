/* tce_tiny.cpp
 *
 * Stripped down version of my larger tail-call-elimination.cpp demo/benchmark
 * program, using only the scheme I chose to use in the VM.
 *
 * MCM 20160131 - original version
 */
#include <stdio.h>
#include <chrono>

typedef void (*VOP)(void);
VOP OPARRAY[100005];
int f1, f2, f3, f4;
int IP;

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
void func4(void) {
    ++f4;
    if (f4>100000)
        return;
    OPARRAY[IP=0]();
}

int main(int, char**) {
    using namespace std::chrono;
    for (int i=0; i<10000; ++i) {
        if (i%17==0) { OPARRAY[i]=&func3; }
        else if (i%13) { OPARRAY[i]=&func2; }
        else {OPARRAY[i]=&func1; }
    }
    OPARRAY[10000]=&func4;
    puts("BOOM!");
    IP=0;
    steady_clock::time_point start = std::chrono::steady_clock::now();
    OPARRAY[IP]();
    steady_clock::time_point end = std::chrono::steady_clock::now();
    duration<double> span = duration_cast<duration<double>>(end-start);
    printf("KERBLAM! IP:%u, f1:%u, f2:%u, f3:%u, f4:%u\n%f seconds\n",
            IP, f1, f2, f3, f4, span.count());
}
