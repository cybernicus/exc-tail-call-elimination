# Makefile
#
# I don't use default rules in make, so if you have any difficulties building
# this, try "make -r".  (I find things a little less surprising this way.)
#
CC    = clang
CC_FL = -std=gnu++11 -O
CC_TCE= -foptimize-sibling-calls
CC_NTCE= -fno-optimize-sibling-calls
LD_FL = -lstdc++

# By default, build everything
all: tce_tiny_slow.exe tce_tiny_fast.exe tce_tiny_fast.S tce_tiny_slow.S

# The slow version with no optimization

tce_tiny_slow.exe: tce_tiny.cpp Makefile
	$(CC) $(CC_FL) $(CC_NTCE) tce_tiny.cpp -o tce_tiny_slow.exe $(LD_FL)

tce_tiny_slow.S: tce_tiny.cpp Makefile
	$(CC) $(CC_FL) $(CC_NTCE) -S tce_tiny.cpp -o tce_tiny_slow.S

# The fast version, using tail-call elimination

tce_tiny_fast.exe: tce_tiny.cpp Makefile
	$(CC) $(CC_FL) $(CC_TCE)    tce_tiny.cpp -o tce_tiny_fast.exe $(LD_FL)

tce_tiny_fast.S: tce_tiny.cpp Makefile
	$(CC) $(CC_FL) $(CC_TCE) -S tce_tiny.cpp -o tce_tiny_fast.S

# I use cygwin under windows, linux and a Mac, so I can get EXE files as well
# as normal *nix executables.
clean:
	rm -f tce_tiny_fast.exe tce_tine_slow.exe tce_tiny.S

