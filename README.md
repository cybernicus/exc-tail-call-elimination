# Example [C]: Tail call elimination

Trivial example of a C program that uses tail call elimination.  Some time ago,
I wrote a compiler/executor for a program constraint analyzer/simulator based
on graphs.  I implemented the executor as a threaded virtual machine and had a
few performance hacks in it to get to the execution speed I needed for my
project.  I was successful in hitting my performance target, though I believe
that there's still some performance to be gained.

This project is simply a demo of one of the optimization techniques I used to
hit my performance target.  It's simply the use of tail-call elimination.  I
built this project because I left the company I was with, so I couldn't keep
the source code of my compiler/executor.  So this project is a simple example
of tail-call elimination that I can use to quickly-refresh my memory for use
in future projects.

OVERVIEW

In a virtual machine, you build a 'simulated computer' with opcodes
that simplify the expression of the low-level concepts you're trying to work
with.  That lets you simplify your compiler a good deal, and make good tradeoffs
between expressibility and performance in the end.

When you have a virtual machine, though, you tend to have a good bit of overhead
to manage the machine operation.  If you elect to create a bytecode oriented
machine, then for each byte you need to perform a byte to function lookup to
determine which function to execute.  For high-level concepts that may not be a
big deal.  But when many of your functions are small and quick, the bytecode
translation itself can be expensive.  So I elected to instead use a threaded
implementation, where each opcode would instead be the function pointers themselves.
This way, you skip the byte code translation at the cost of having each token be
the size of a pointer instead of the size of a byte.

The programs I was dealing with were small enough that the tradeoff was very
worthwhile, so I didn't view it as a cost at all.  That reduced the overhead, but
left one other big one: stack frame setup/teardown.  I was using a stack-oriented
machine, so my opcodes generally had very few arguments.  In fact, there were
only a handful (less than 5) that used more than one argument.

While reading the clang docs, I noticed it supported tail-call elimination
(via the -foptimize-sibling-calls flag) which allows us to totally get rid of
all stack frame construction/teardown and instead use jumps between function
bodies.  It's easy enough to use, but there are a couple conditions:

 1) Your function has to *end* with the call to the next function.
 2) The functions need to have the same prototype.

The first one is the easiest to manage, as generally you simply just call the
next opcode at the end of your function.  The second one is a *little* trickier:
what do you do when you have different types of opcodes?  In my case, I had
some that took *no* arguments, some that took one, and a few that took more
than one.  Additionally, of the functions that took arguments, there were
different types!  How to use a single function prototype to accomodate them all?

I did this by taking advantage of the fact that my VM was executing code that
was generated by my compiler, and that all the opcodes *knew* the data types
that they could use.  So by relying on the compiler to always generate correct
code, I could offload the burden of dealing with the number and type of arguments
to the opcodes that needed to care.

See the internal documentation in tce_tiny.cpp for details.  In brief, I took
the course of telling the compiler that *all* opcodes are pointers to functions
that took no arguments and returned nothing:

    typedef void (*VOP)(void);

All the code was stored in a static array.  Since the programs generated by the
executor/simulator are small, it was a simple static array with 100k steps.
There are also several "register" variables that the virtual machine can use as
immediate values.

    // Our program (opcodes, arguments, and stack) must fit in our program
    // buffer
    VOP OPARRAY[100005];

    // The "registers" in our virtual machine:
    int IP,     // Program counter
        SP,     // Stack pointer
        FLAGS,  // Flags from operations (ZR=zero, NZ=not zero, ...)

The IP register (Instruction Pointer) tells us where we are in the execution of
our program.  At the start of each opcode, IP holds the slot number on OPARRAY
that represents the current instruction.  By the end of the opcode handler, it
must point to the *next* instruction slot to execute.

The SP register (Stack Pointer) tells us which slot is the top of our stack.
As is common practice, our program starts at the beginning of our memory space,
and the stack starts at the end of our memory space.

So, then, handling opcodes with no arguments is pretty trivial:  We simply
provide a function with the correct signature that does its work.  At the end
of the function, we pass control to the function pointed to by the next slot in
the program buffer:

    // The instruction after a no-arguments opcode has to:
    //      increment the IP register,
    //      fetch the pointer from the next OPARRAY slot, and
    //      invoke that function
    #define NEXT_INSTRUCTION OPARRAY[++IP]

    void opcode_FOO(void) {
        // Do the "FOO" stuff
        . . .

        NEXT_INSTRUCTION();
    }

For opcodes that use arguments, however, I provided a set of macros that would
fetch the appropriate argument from OPARRAY and cast it to the specified type.
Additionally, I created a couple macros that would go to the next instruction
based on the number of arguments required by the opcode.

    // Get argument 'idx' as an integer.
    #define GET_ARG_INT(idx) (int)(OPARRAY[IP+idx])

    // Advance IP to the start of the next instruction, and execute it
    #define NEXT_INSTR(num_args) IP += num_args; OPARRAY[IP]()

    void opcode_PUSH_INT_ON_STACK(void) {
        // Fetch argument 1 as an integer:
        int tmp = GET_ARG_INT(1);

        // Store an integer on the top of the stack
        OPARRAY[--SP] = INT_TO_VOP(tmp);

        // skip one argument, execute the next instruction
        NEXT_INSTR(1);
    }

Note that the method shown requires that we don't change IP until we're done
fetching all the arguments.  I also experimented with some macros that would
each fetch the next argument from the OPARRAY, so each time we'd fetch an
argument, IP would automatically advance.  I don't recall which one I settled
on in the end, though.  I did some performance testing and found cases where
each method was faster than the other, but I don't recall which one won overall.

I remember only a couple things about the differences:
    1)  The version that fetched each operand in turn and updated IP was kind
        of nice because the macros were simpler, and I didn't need another
        NEXT_INSTRUCTION macro, because by the time all arguments were
        fetched, the next instruction was already pointed to by IP
    2)  You could get a speed bump with opcodes that had optional arguments
        when you used the GET_ARG_type(idx) form, but you had to be more careful
        when fetching arguments and adjusting IP.

The only other item I think is interesting are the opcodes that would change
the control flow of your program.  You'd have conditional branches where you'd
check a flag and then either advance to the next instruction *or* the optional
argument.  Subroutines were a little more complex, in that the call instruction
had to put the address of the next instruction on top of the stack, and the
return instruction would have to pop the return address from the stack and then
go to that instruction.

Keep in mind that you need to ensure that the last instruction in an opcode
handling function is the call to the next opcode to take advantage of tail-call
elimination.  So if I recall correctly, the flow control instructions structured
themselve something like this:

    // Jump to label if zero flag set
    void op_JMPZ(void) {
        VOP label = OPARRAY[IP+1];  // label is first argument
        VOP next = OPARRAY[IP+2];   // next is instruction *after* first argument

        // Jumps are easy: we don't need to do anything beyond select the next
        // address to execute
        IP = (FLAGS & ZR) ? label : next;

        // Go to the instruction
        OPARRAY[IP]();
    }

    // Call subroutine if zero flag set
    void op_CALLZ(void) {
        VOP label = OPARRAY[IP+1];
        VOP next = OPARRAY[IP+2];

        if (FLAGS & ZR) {
            // Condition is true, so push 'next' then execute 'label'
            OPARRAY[--SP] = next;
            IP = label;
        }
        else {
            // Condition is false, so we'll just execute 'next'
            IP = next;
        }

        // Go to the instruction
        OPARRAY[IP]();
    }

    // Return from subroutine if zero flag set
    void op_RETZ(void) {
        if (FLAGS & ZR) {
            // Condition is true, pop next instruction from stack
            IP = OPARRAY[SP++];
        }
        else {
            // Condition is false, just execute next instruction
            IP++;
        }

        // Go to the instruction
        OPARRAY[IP]();
    }

RESULTS

I created a stupid program to let you see the performance difference
you get when tail-call elimination is on or off.  However, since the
program is executed as a *very deep* recursive function without tail-call
elimination, you need to run with very small values of PGM_SIZE and
PGM_LOOPS.  So for showing the difference between the optimization on
and off, I've set PGM_SIZE and PGM_LOOPS to 100 and 10 respectively.
This gives me the following results in a desktop PC running Windows 8.1
on an i5-4670K CPU:

        # ins       # ins/sec       wall-clock time
slow:   1111        44,443,555       0.026s
fast:   1111       409,811,877       0.023s

So the tail-call elimination optimization gives us a nearly 10 times
speedup.  Note: the opcodes run didn't do much, so it doesn't mean that
it made my virtual machine 10 times faster.  Instead it reduced the
amount of overhead dramatically.  So while my VM wasn't 10 times faster,
it was still *quite* a lot faster after I made this optimization.

With the default values of PGM_SIZE=10000 and PGM_LOOPS=100000, running
the fast version gives the following output, showing that the opcode
overhead is low enough to get nearly 550 million opcodes per second:

    $ time ./tce_tiny_fast.exe
    BOOM!
    KERBLAM! IP:10000, f1:72400724, f2:868708687, f3:58900589, f4:100001
    1.822515 seconds
    instructions executed: 1000110001, instructions/second: 548752670.054766

    real    0m1.844s
    user    0m1.828s
    sys     0m0.015s

The real-world results were pretty good in the actual VM.  The final version
I ran, where the opcodes actually did significant work yielded just over
8 million operations per second in the graph machine.  We were doing various
stack operations, graph-trace operations, etc.  While some of those operations
consumed a decent bit of work, there were enough 'simple' operations in most
programs to make most programs hit the 8 million opcodes/sec mark, with few
programs being much slower than that.


NOTE: While it's a C example, my makefile specifies C++ '11.  Just a
habit of mine--I pretty much always use C++ or C-style under C++.

LICENSE: There's absolutely no IP in here, as it's just an example of
information you can already find anywhere on the net.  I just packed
it into a format where I can get it easily when I forget.  So feel free
to use anything you like in this example in any way you'd like to.

