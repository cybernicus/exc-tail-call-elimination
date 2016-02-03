# Makefile
#
# I don't use default rules in make, so if you have any difficulties building
# this, try "make -r".  (I find things a little less surprising this way.)
#
CC    = clang
CC_FL = -std=gnu++11 -O
CC_TCE= -foptimize-sibling-calls
LD_FL = -lstdc++

tce_tiny: tce_tiny.cpp Makefile
	$(CC) $(CC_FL) $(CC_TCE) tce_tiny.cpp -o tce_tiny $(LD_FL)

# I use cygwin under windows, linux and a Mac, so I can get EXE files as well
# as normal *nix executables.
clean:
	rm -f tce_tiny.exe tce_tiny


