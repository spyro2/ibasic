/* Sieve of Eratosthenes */
/* Classic prime number finding algorithm */
/* https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes */

size% = 10000

DIM A%[size%+1]

FOR I% = 2 TO size%
	A%[I%] = 0
NEXT

FOR I% = 2 TO size%
	FOR J% = I%*I% TO size% STEP I%
		A%[J%] = 1
	NEXT
NEXT

PRINT "Primes up to ";size%;":"

C% = 0
FOR I% = 2 TO size%
	IF A%[I%] = 0 THEN PRINT I%;",": C% = C%+1
NEXT

PRINT "Total: ";C%
