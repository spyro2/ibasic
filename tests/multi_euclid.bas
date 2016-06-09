/* An implementation of the Euclid's Algorithm       */
/* that loops from 252 up to 1000000 */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

A%=252
B%=105

REPEAT
	PRINT "Greatest common divisor of "; A%; " and "; B%; " is: "; FNeuclid(A%, B%)
	A% = A% + 1

UNTIL A% = 1000000

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

