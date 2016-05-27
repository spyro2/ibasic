/* First comment */
LIBRARY "graphics.bas"

PRINT 37
PRINT 0xfead0123
PRINT 0744
PRINT 3.14159

A = -3 + 4 * 2 / ( 1 - 5 )
A = 3 + 4 * 2 / ( 1 - 5 )
A = 3 + 4 * 2 / ( FNdoit(A, B, C, D) - 5 )
A = 3 + 4 * 2 / ( -FNdoit(A, B, C, D) - 5 )
A = 3 + 4 * 2 / ( FNdoit(A, B, 4*g-(a*-3)) - 5 )
A = 3 + 4 * 2 / ( FNdoit(A, FNdothat(B,3*4-a), 4*g-(a*-3)) - 5 )
B = -B+9*3+A--2*-d
B = -B+9*3*A--2*-d

/* From wikipedia - ((15 ÷ (7 − (1 + 1))) × 3) − (2 + (1 + 1)) */
/* The above should evaluate to: - + + {1} {1} {2} * {3} / - + {1} {1} {7} {15} */
/* IOW, 5 */

l = ((15 / (7 - (1 + 1))) * 3) - (2 + (1 + 1))

A = 4*g-(a*-3)
A = 4*g-(-a*-3)

PRINT:
l = d
:PRINT

PRINT "Hello World!"
IF A = 7 THEN PRINT A ELSE PRINT "sharks!":L=7
IF A = 7 PRINT A ELSE PRINT "sharks!" ENDIF:L=7

A = +7-(+b-3) /* Set A to 7 */

STATIC A:
GLOBAL B = 8:PRINT B
CONST C

IF A = 7 THEN PRINT A:PRINT:PRINT B;C
IF A = 7 THEN PRINT A ENDIF
IF A = -(-2*bob+3)*7 THEN PRINT A ENDIF:PRINT A
IF A = 7 THEN PRINT A ELSE PRINT "sharks!" ENDIF
IF A = 7 THEN /*PRINT A*/ PRINT B ELSE PRINT "sharks!" ENDIF
IF A = 7 THEN PRINT A*3 ELSE PRINT "sharks!";sharks*5; ENDIF: PRINT:PRINT A:PRINT B;C

B = -B+9*3+A-(-2*-d)

IF num=37 THEN
  PRINT "Number was 37"
  num = 38
ELSE
  num=A
  PRINT "num set to A";A;
ENDIF

IF num=37 THEN
  PRINT "Number was 37"
  num = 38
ELSE IF B=4 THEN
    num=A
    PRINT "num set to A";A;
  ENDIF
ENDIF

IF num=37 THEN
  PRINT "Number was 37"
  num = 38
ELSE IF B=4 THEN
    num=A
    PRINT "num set to A";A;
  ELSE
    PRINT "or not..."
  ENDIF
ENDIF

PROCfoo
PROCbar:PROCbaz(A, (B*dogs)/5)
PROCbar:PROCbaz(A, B)/*PRINT a*/
PROCbar:PROCbaz(A, /*B*/C)/*PRINT a*/:PRINT B
PROCbar:PROCbaz(A, B):PRINT a
FNgetreg(reg)

DEF PROCfoo:PRINT
ENDPROC

DEFPROCbaz(c, RETURN d )
PRINT a;" ";d
ENDPROC

DEFPROCbar
PRINT "Hi!"
ENDPROC

DEFPROC whizz(m):PRINT F
ENDPROC

DEF FN getreg(reg)
PRINT "Get reg ";reg
=reg

REPEAT
A = B
	WHILE D
		PRINT E
		IF A THEN PROCbar ELSE PROCbaz(0, 8)
		IF cats=cool GOTO herelabel
	ENDWHILE
UNTIL C = 9

herelabel:

REPEAT:PRINT fun:UNTIL 2=h:PRINT bags
WHILE boo=argh: PRINT wheee:ENDWHILE:GOTO label

WHILE 1
	PRINT "Endless"
ENDWHILE

CASE FNdothing(A, 5, (3-6*(5-b)/(76-A*n))) OF
	WHEN 7,8,9: PRINT A:IF foo=8 PRINT "Yay" ELSE PRINT "boooo":PRINT B
	WHEN 10
		PRINT "HI!"
		PRINT "Bye":PRINT:PRINT:PRINT "googley eyes"
	WHEN B+8
		REPEAT
		A = B
			WHILE D
				PRINT E
				IF A THEN PROCbar ELSE PROCbaz(0, 8)
			ENDWHILE
		UNTIL C
	OTHERWISE
		IF A THEN PRINT "Dog":
ENDCASE

END

IF 7 THEN
	PRINT "Foo!"
ELSE
	PRINT "BAR!"
ENDIF

label:
SUM = A+B
variable = 3+7*4+(7-5/2)
PRINT A; B; SUM; "SUM THING!"

C = 0
FOR C = 0 TO 1000 STEP 10
  PRINT C
NEXT


DIM A[3][3][3] AS SIGNED SHORT



DEF VESSEL_INFO{name$, year%, mass}

DEF VOYAGE{destination$, boat AS VESSEL_INFO, completed AS BIT}



DIM trip(50) AS VOYAGE

trip(0) = {"Spain", {"HMS Blue Lagoon", 1978, 0.78}, 1}




LIST voyages AS VOYAGE

LET new_voyage BE VOYAGE
new_voyage.destination = "France"
new_voyage.boat.name="RRS Boaty Mc Boatface"
LIST ADD new_voyage AFTER voyages

FOR EACH voyage IN voyages
	PRINT "Destination: ";voyage.destination
NEXT




DIM Grade%(5)
      
FOR I%=1 TO 5
  PRINT "Enter grade for student ";I%;": ";
  INPUT Grade%(I%)
NEXT I%
      
Total%=0
FOR I%=1 TO 5
  Total%=Total%+Grade%(I%)
NEXT I%
PRINT "The average grade was ";Total%/5
      
Minimum%=999
FOR I%=1 TO 5
  IF Grade%(I%)<Minimum% THEN
    Minimum%=Grade%(I%)
  ENDIF
NEXT I%
PRINT "The minimum grade was ";Minimum%
      
A$ = LEFT$("Fred has a farm", 8)
LEFT$(A$, 4) = "Bob owns a Ferrari")


END
