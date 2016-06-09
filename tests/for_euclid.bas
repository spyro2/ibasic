/* An implementation of the Euclid's Algorithm       */
/* This version is intended to test iBASIC's FOR loops */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

FOR A% = 200 + FNeuclid(252,105) TO 10000
	FOR B% = 150 TO 20 STEP -7
		PRINT "Greatest common divisor of "; A%; " and "; B%; " is: "; FNeuclid(A%, B%)
	NEXT
NEXT

END

DEF FNeuclid(A%, B%)
	WHILE B% <> 0
		IF A% > B% THEN
			A% = A% - B%
		ELSE
			B% = B% - A%
		ENDIF
	ENDWHILE
=A%

