/* Test the implementation of CASE */

C% = 11

FOR A% = 17 TO 0 STEP -1
	CASE FNthing(A%) OF
		WHEN 0, 3, 6:
			PRINT "It's"
		WHEN 1, 4, 15:
			PRINT "Pancake"
		WHEN 2, 5:
			PRINT "Day,"
		WHEN 16:
			PRINT "Day"
		WHEN 7, 8, 9, FNten(2, C%), C%, 12, 13, 14:
			PRINT "P-"
		OTHERWISE: PRINT "."
	ENDCASE
NEXT

END

DEF FNten(A%, B%)
=2*(B%-6)

DEF FNthing(A%)
=17-A%
