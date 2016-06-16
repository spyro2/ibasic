/* A function to determine primality */

size% = 100

PRINT "Primes up to ";size%;":"

FOR I% = 2 TO size%
	IF FNisprime(I%) = 1 THEN PRINT I%;","
NEXT

END

DEF FNisprime(C%)
	P% = 1
	IF C% > 2 THEN
		FOR L% = 2 TO C%-1
			IF FNremain(C%, L%) = 0 THEN
				P% = 0
				BREAK
			ENDIF
		NEXT
	ENDIF
=P%

DEF FNremain(A%, B%)
	C% = A%/B%
	C% = C%*B%
=A%-C%
