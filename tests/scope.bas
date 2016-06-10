/* Test scoping */
/* Try removing declarations and see if the interpreter works as expected. */

A% = 666

PROCscope
FNdoberry
PRINT A%

/* Will print no such variable */
PRINT B%

END

DEF FNdoberry
A% = 555
B% = 666
PRINT FNscope2
=9

DEF PROCscope
A% = 555
B% = 666
PRINT FNscope2
ENDPROC

DEF FNscope2
/* Removing the below will print no such variable */
B%=8
PRINT A%
PRINT B%
=7
