/* Loop tests */

FOR I% = 0 TO 100
	IF I% = 4 THEN PRINT "skip 4": CONTINUE
	PRINT I%
	IF I% = 8 THEN BREAK
NEXT

I% = -1
WHILE I% <= 100
	I% = I% + 1
	IF I% = 4 THEN PRINT "skip 4": CONTINUE
	PRINT I%
	IF I% = 8 THEN BREAK
ENDWHILE

I% = -1
REPEAT
	I% = I% + 1
	IF I% = 4 THEN PRINT "skip 4": CONTINUE
	PRINT I%
	IF I% = 8 THEN BREAK
UNTIL I% >= 100