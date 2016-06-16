/* Test the LOCAL statement */

A% = 4

PRINT "Calling shadowed version"
PROCshadow(A%, 6)

PRINT "Should be 4: ";A%

PRINT "Calling unshadowed version"
PROCno_shadow(A%, 6)

PRINT "Should be 8: ";A%


END

DEFPROCshadow(C%, D%)
	LOCAL A%

	A% = D%

	PRINT "Should be 4: ";C%
	PRINT "Should be 6: ";A%
	A% = 8
ENDPROC

DEFPROCno_shadow(C%, D%)
	PRINT "Should be 4: ";C%
	PRINT "Should be 4: ";A%
	A% = 8
ENDPROC
