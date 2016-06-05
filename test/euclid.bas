/* An implementation of the Euclid's Algorithm       */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

A%=252
B%=105

PRINT "Greatest common divisor of "; A%; " and "; B%; " is: "; FNeuclid(A%, B%)

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

