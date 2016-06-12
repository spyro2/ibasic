

REPEAT
	PRINT "HELLO!"

	PROCthing(100)
	PRINT FNthingy(100)

UNTIL FNrecurse(4) = 0

END

DEF FNthingy(A%)
	PRINT A%
=9

DEF PROCthing(A%)
	PRINT A%
ENDPROC

DEF FNrecurse(A%)
	PRINT A%
	IF A% > 0 THEN FNrecurse(A%-1): PRINT "A%= ";A%: A% = FNrecurse(A%-1)
=A%
