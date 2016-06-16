/* Test the LOCAL statement */

DIM A%[6]

A%[3] = 4

PRINT "Calling shadowed version"
PROCshadow(A%, 6)

PRINT "Should be 4: ";A%[3]

PRINT "Calling unshadowed version"
PROCno_shadow(A%, 6)

PRINT "Should be 8: ";A%[3]

END

DEFPROCshadow(C%[], D%)
	LOCAL A%[]
	DIM A%[4]

	A%[3] = D%

	PRINT "Should be 4: ";C%[3]
	PRINT "Should be 6: ";A%[3]
	A%[3] = 8
ENDPROC

DEFPROCno_shadow(C%[], D%)
	PRINT "Should be 4: ";C%[3]
	PRINT "Should be 4: ";A%[3]
	A%[3] = 8
ENDPROC
