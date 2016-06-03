/* An implementation of the Euclid's Algorithm       */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

A%=252
B%=105

PRINT "Greatest common divisor of "; A%; " and "; B%; " is:"; FNeuclid(A%, B%)

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

