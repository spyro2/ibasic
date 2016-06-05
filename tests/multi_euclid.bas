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

DEF FNeuclid(C%, D%)
	WHILE D% <> 0
		IF C% > D% THEN
			C% = C% - D%
		ELSE
			D% = D% - C%
		ENDIF
	ENDWHILE
=C%

