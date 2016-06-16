/* Some tests of array handling */

DIM A%[5]

A%[4] = 0x452

PRINT FNthing(A%, 4); " "; 0x452

END

DEF FNthing(A%[], B%)
=A%[B%]
