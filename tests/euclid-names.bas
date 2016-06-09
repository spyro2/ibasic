/* An implementation of the Euclid's Algorithm       */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

A%=252
B%=105

PRINT "Greatest common divisor of "; A%; " and "; B%; " is: "; FNeuclid(A%, B%)

END

DEF FNeuclid(first%, second%)
	WHILE second% <> 0
		IF first% > second% THEN
			first% = first% - second%
		ELSE
			second% = second% - first%
		ENDIF
	ENDWHILE
=first%

