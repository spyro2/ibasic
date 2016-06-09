/* An implementation of the Euclid's Algorithm       */
/* that loops from 252 up to 1000000 */

/* https://en.wikipedia.org/wiki/Euclidean_algorithm */

B% = 105

FOR I% = 252 TO 999999
	PRINT "Greatest common divisor of "; I%; " and "; B%; " is: "; FNeuclid(I%, B%)
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

