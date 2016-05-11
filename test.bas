PRINT "Hello World!"
IF A = 7 THEN PRINT A ELSE PRINT "sharks!"
A = 7
B = 9

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
PROCbar:PROCbaz(A, B)
PROCbar:PROCbaz(A, B):PRINT a
FNgetreg(reg)

DEF PROCfoo
ENDPROC

DEFPROCbaz(c, d )
PRINT a;" ";d
ENDPROC

DEFPROCbar
PRINT "Hi!"
ENDPROC

DEF FN getreg(reg)
PRINT "Get reg ";reg
=reg

CASE foo OF
	WHEN 7,8,9: PRINT A:IF foo=8 PRINT "Yay" ELSE PRINT "boooo":PRINT B
	WHEN 10
		PRINT "HI!"
		PRINT "Bye":PRINT:PRINT:PRINT "googley eyes"
	OTHERWISE
		IF A THEN PRINT "Dog":
ENDCASE

END

IF 7 THEN
	PRINT "Foo!"
ELSE
ELSE
	PRINT "BAR!"
ENDIF

label:
SUM = A+B
variable = 3+7*4+(7-5/2)
PRINT A B SUM "SUM THING!"

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
