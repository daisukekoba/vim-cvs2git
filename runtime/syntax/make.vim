" Vim syntax file
" Language:	Makefile
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/make.vim
" Last Change:	2000 Nov 12

" Remove any old syntax stuff hanging around
syn clear

" some directives
syn match makePreCondit	"^\s*\(ifeq\>\|else\>\|endif\>\|define\>\|endef\>\|ifneq\>\|ifdef\>\|ifndef\>\)"
syn match makeInclude	"^\s*include"
syn match makeStatement	"^\s*vpath"
syn match makeOverride	"^\s*override"
hi link makeOverride makeStatement

" Microsoft Makefile specials
syn case ignore
syn match makeInclude	"^!\s*include"
syn match makePreCondit "!\s*\(cmdswitches\>\|error\>\|message\>\|include\>\|if\>\|ifdef\>\|ifndef\>\|else\>\|elseif\>\|else if\>\|else\s*ifdef\>\|else\s*ifndef\>\|endif\>\|undef\>\)"
syn case match

" identifiers
syn region makeIdent	start="\$(" skip="\\)" end=")" contains=makeStatement,makeIdent
syn region makeIdent	start="\${" skip="\\}" end="}" contains=makeStatement,makeIdent
syn match makeIdent	"\$\$\w*"
syn match makeIdent	"\$[^({]"
syn match makeIdent     "^\s*\a\w*\s*[:+?!*]="me=e-2
syn match makeIdent	"^\s*\a\w*\s*="me=e-1
syn match makeIdent	"%"


" make targets
syn match makeSpecTarget	"^\.SUFFIXES"
syn match makeSpecTarget	"^\.PHONY"
syn match makeSpecTarget	"^\.DEFAULT"
syn match makeSpecTarget	"^\.PRECIOUS"
syn match makeSpecTarget	"^\.IGNORE"
syn match makeSpecTarget	"^\.SILENT"
syn match makeSpecTarget	"^\.EXPORT_ALL_VARIABLES"
syn match makeSpecTarget	"^\.KEEP_STATE"
syn match makeSpecTarget	"^\.LIBPATTERNS"
syn match makeSpecTarget	"^\.NOTPARALLEL"
syn match makeImplicit	        "^\.[A-Za-z0-9_./\t -]\+\s*:[^=]"me=e-2
syn match makeImplicit	        "^\.[A-Za-z0-9_./\t -]\+\s*:$"me=e-1
syn match makeTarget		"^[A-Za-z0-9_./$()%-][A-Za-z0-9_./\t $()%-]*:[^=]"me=e-2 contains=makeIdent
syn match makeTarget		"^[A-Za-z0-9_./$()%-][A-Za-z0-9_./\t $()%-]*:$"me=e-1 contains=makeIdent

" Statements / Functions (GNU make)
syn match makeStatement contained "(subst"ms=s+1
syn match makeStatement contained "(addprefix"ms=s+1
syn match makeStatement contained "(addsuffix"ms=s+1
syn match makeStatement contained "(basename"ms=s+1
syn match makeStatement contained "(call"ms=s+1
syn match makeStatement contained "(dir"ms=s+1
syn match makeStatement contained "(error"ms=s+1
syn match makeStatement contained "(filter"ms=s+1
syn match makeStatement contained "(filter-out"ms=s+1
syn match makeStatement contained "(findstring"ms=s+1
syn match makeStatement contained "(firstword"ms=s+1
syn match makeStatement contained "(foreach"ms=s+1
syn match makeStatement contained "(if"ms=s+1
syn match makeStatement contained "(join"ms=s+1
syn match makeStatement contained "(notdir"ms=s+1
syn match makeStatement contained "(origin"ms=s+1
syn match makeStatement contained "(patsubst"ms=s+1
syn match makeStatement contained "(shell"ms=s+1
syn match makeStatement contained "(sort"ms=s+1
syn match makeStatement contained "(strip"ms=s+1
syn match makeStatement contained "(suffix"ms=s+1
syn match makeStatement contained "(warning"ms=s+1
syn match makeStatement contained "(wildcard"ms=s+1
syn match makeStatement contained "(word"ms=s+1
syn match makeStatement contained "(words"ms=s+1

" some special characters
syn match makeSpecial	"^\s*[@-]\+"
syn match makeNextLine	"\\$"


" Errors
syn match makeError     "^ \+\t"
syn match makeError     "^ \{8\}[^ ]"me=e-1
syn region makeIgnore	start="\\$" end="^." end="^$" contains=ALLBUT,makeError

" Comment
syn region  makeComment	start="#" end="[^\\]$"
syn match   makeComment	"#$"

" match escaped quotes and any other escaped character
" except for $, as a backslash in front of a $ does
" not make it a standard character, but instead it will
" still act as the beginning of a variable
" The escaped char is not highlightet currently
syn match makeEscapedChar 	"\\[^$]"


syn region  makeDString start=+"+  skip=+\\"+  end=+"+  contains=makeIdent
syn region  makeSString start=+'+  skip=+\\'+  end=+'+  contains=makeIdent
syn region  makeBString start=+`+  skip=+\\`+  end=+`+  contains=makeIdent,makeSString,makeDString,makeNextLine

" The default highlighting.
hi def link makeNextLine	makeSpecial
hi def link makeSpecTarget	Statement
hi def link makeImplicit	Function
hi def link makeTarget		Function
hi def link makeInclude		Include
hi def link makePreCondit	PreCondit
hi def link makeStatement	Statement
hi def link makeIdent		Identifier
hi def link makeSpecial		Special
hi def link makeComment		Comment
hi def link makeDString		String
hi def link makeSString		String
hi def link makeBString		Function
hi def link makeError		Error

let b:current_syntax = "make"

" vim: ts=8
