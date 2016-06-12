/* Test wether function and procedure names can be different */

PRINT FNthingy(7)
PROCthingy(5)

END

DEFPROCthingy(A%)
	PRINT A%
ENDPROC

DEF FNthingy(A%)
	PRINT "FN "; A%
=A% * 2
