" Vim syntax file
" Language:	bc - An arbitrary precision calculator language
" Maintainer:	Vladimir Scholtz <vlado@gjh.sk>
" Last change:	2000 Nov 04
" Available on:	www.gjh.sk/~vlado/bc.vim

" Remove any old syntax
syn clear
syn case ignore

" Keywords
syn keyword bcKeyword if else while for break continue return limits halt quit
syn keyword bcKeyword define
syn keyword bcKeyword length read sqrt print

" Variable
syn keyword bcType auto

" Constant
syn keyword bcConstant scale ibase obase last
syn keyword bcConstant BC_BASE_MAX BC_DIM_MAX BC_SCALE_MAX BC_STRING_MAX
syn keyword bcConstant BC_ENV_ARGS BC_LINE_LENGTH

" Any other stuff
syn match bcIdentifier		"[a-z_][a-z0-9_]*"

" String
 syn match bcString		"\"[^"]*\""

" Number
syn match bcNumber             "[0-9]\+"

" Comment
syn match bcComment		"\#.*"
syn region bcComment		start="/\*" end="\*/"

" Parent ()
syn cluster bcAll contains=bcList,bcIdentifier,bcNumber,bcKeyword,bcType,bcConstant,bcString,bcParentError
syn region bcList             matchgroup=Delimiter start="(" skip="|.\{-}|" matchgroup=Delimiter end=")" contains=@bcAll
syn region bcList             matchgroup=Delimiter start="\[" skip="|.\{-}|" matchgroup=Delimiter end="\]" contains=@bcAll
syn match bcParenError                  "]"
syn match bcParenError                  ")"



syn case match

" The default highlighting.
hi def link bcKeyword		Statement
hi def link bcType		Type
hi def link bcConstant		Constant
hi def link bcNumber		Number
hi def link bcComment		Comment
hi def link bcString		String
hi def link bcSpecialChar	SpecialChar
hi def link bcParenError	Error

let b:current_syntax = "bc"
" vim: ts=8
