" Vim syntax file
" Language:     PIC16F84 Assembler (Microchip's microcontroller)
" Maintainer:   Aleksandar Veselinovic <alexa@cliffhanger.com>
" Last Change:  2000/08/28
" URL:          http://SOLAIR.EUnet.yu/~aleksav/vim/syntax/pic.vim
" Revision:     1.0

" Remove any old syntax stuff hanging around
syn clear

syn case match
syn keyword picTodo NOTE TODO XXX contained


syn case ignore

syn match picIdentifier "[a-z_$][a-z0-9_$]*"
syn match picLabel      "^[A-Z_$][A-Z0-9_$]*"
syn match picLabel      "^[A-Z_$][A-Z0-9_$]*:"me=e-1

syn match picASCII      "A\='.'"
syn match picBinary     "B'[0-1]\+'"
syn match picDecimal    "D'\d\+'"
syn match picDecimal    "\d\+"
syn match picHexadecimal "0x\x\+"
syn match picHexadecimal "H'\x\+'"
syn match picHexadecimal "[0-9]\x*h"
syn match picOctal      "O'[0-7]\o*'"


syn match picComment    ";.*" contains=picTodo

syn region picString    start=+"+ end=+"+

syn keyword picRegister         INDF TMR0 PCL STATUS FSR PORTA PORTB
syn keyword picRegister         EEDATA EEADR PCLATH INTCON INDF OPTION_REG PCL
syn keyword picRegister         FSR TRISA TRISB EECON1 EECON2 INTCON OPTION


" Register --- bits

" STATUS
syn keyword picRegisterPart     IRP RP1 RP0 TO PD Z DC C

" PORTA
syn keyword picRegisterPart     T0CKI
syn match   picRegisterPart     "RA[0-4]"

" PORTB
syn keyword picRegisterPart     INT
syn match   picRegisterPart     "RB[0-7]"

" INTCON
syn keyword picRegisterPart     GIE EEIE T0IE INTE RBIE T0IF INTF RBIF

" OPTION
syn keyword picRegisterPart     RBPU INTEDG T0CS T0SE PSA PS2 PS1 PS0

" EECON2
syn keyword picRegisterPart     EEIF WRERR WREN WR RD

" INTCON
syn keyword picRegisterPart     GIE EEIE T0IE INTE RBIE T0IF INTF RBIF


" OpCodes...
syn keyword picOpcode  ADDWF ANDWF CLRF CLRW COMF DECF DECFSZ INCF INCFSZ
syn keyword picOpcode  IORWF MOVF MOVWF NOP RLF RRF SUBWF SWAPF XORWF
syn keyword picOpcode  BCF BSF BTFSC BTFSS
syn keyword picOpcode  ADDLW ANDLW CALL CLRWDT GOTO IORLW MOVLW RETFIE
syn keyword picOpcode  RETLW RETURN SLEEP SUBLW XORLW 
syn keyword picOpcode  GOTO


" Directives
syn keyword picDirective __BADRAM BANKISEL BANKSEL CBLOCK CODE __CONFIG
syn keyword picDirective CONSTANT DATA DB DE DT DW ELSE END ENDC
syn keyword picDirective ENDIF ENDM ENDW EQU ERROR ERRORLEVEL EXITM EXPAND
syn keyword picDirective EXTERN FILL GLOBAL IDATA __IDLOCS IF IFDEF IFNDEF
syn keyword picDirective INCLUDE LIST LOCAL MACRO __MAXRAM MESSG NOEXPAND
syn keyword picDirective NOLIST ORG PAGE PAGESEL PROCESSOR RADIX RES SET
syn keyword picDirective SPACE SUBTITLE TITLE UDATA UDATA_OVR UDATA_SHR
syn keyword picDirective VARIABLE WHILE INCLUDE
syn match picDirective   "#\=UNDEFINE"
syn match picDirective   "#\=INCLUDE"
syn match picDirective   "#\=DEFINE"




if !exists("did_pic16f84_syntax_inits")
  let did_pic16f84_syntax_inits = 1

  hi link picTodo               Todo
  hi link picComment            Comment
  hi link picDirective          Statement
  hi link picLabel              Label
  hi link picString             String

  "hi link picOpcode            Keyword
  "hi link picRegister          Structure
  "hi link picRegisterPart      Special

  hi link picASCII              String
  hi link picBinary             Number
  hi link picDecimal            Number
  hi link picHexadecimal        Number
  hi link picOctal              Number

  hi link picIdentifier         Identifier
endif

let b:current_syntax = "pic"

" vim: ts=8
