/* A small test to see if FOR...NEXT handles the index being changed */
/* in the loop */

FOR I% = 10 TO -10 STEP -2
	PRINT I%
	IF I% = -4 THEN I% = -8
NEXT
