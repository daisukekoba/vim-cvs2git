" Vim syntax file
" Language:	sendmail
" Maintainer:	Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change:	June 29, 1999

" Remove any old syntax stuff hanging around
syn clear

" Comments
syn match smComment	"^#.*$"

" Operators
syn match smOper	"$"

" Definitions, Classes, Files, Options, Precedence, Trusted Users, Mailers
syn match smDefine	"^[CDFPT]."
syn match smDefine	"^O[AaBcdDeFfgHiLmNoQqrSsTtuvxXyYzZ]"
syn match smDefine	"^O\s"he=e-1
syn match smDefine	"^M[a-zA-Z0-9]\+,"he=e-1

" Header Format  H?list-of-mailer-flags?name: format
syn match smHeaderSep contained "[?:]"
syn match smHeader	"^H\(?[a-zA-Z]\+?\)\=[-a-zA-Z_]\+:" contains=smHeaderSep

" Variables
syn match smVar		"\$[a-z\.\|]"

" Rulesets
syn match smRuleset	"^S\d*"

" Rewriting Rules
syn match smRewrite	"^R"			skipwhite skipnl nextgroup=smRewriteLhsToken,smRewriteLhsUser

syn match smRewriteLhsUser	contained "[^\t$]\+"		skipwhite skipnl nextgroup=smRewriteLhsToken,smRewriteLhsSep
syn match smRewriteLhsToken	contained "\(\$[-*+]\|\$[-=][A-Za-z]\)\+"	skipwhite skipnl nextgroup=smRewriteLhsUser,smRewriteLhsSep

syn match smRewriteLhsSep	contained "\t\+"			skipwhite skipnl nextgroup=smRewriteRhsToken,smRewriteRhsUser

syn match smRewriteRhsUser	contained "[^\t$]\+"		skipwhite skipnl nextgroup=smRewriteRhsToken,smRewriteRhsSep
syn match smRewriteRhsToken	contained "\(\$\d\|\$>\d\|\$#\|\$@\|\$:[-_a-zA-Z]\+\|\$[[\]]\|\$@\|\$:\|\$[A-Za-z]\)\+" skipwhite skipnl nextgroup=smRewriteRhsUser,smRewriteRhsSep

syn match smRewriteRhsSep	contained "\t\+"			skipwhite skipnl nextgroup=smRewriteComment,smRewriteRhsSep
syn match smRewriteRhsSep	contained "$"

syn match smRewriteComment	contained "[^\t$]*$"

" Clauses
syn match smClauseError		"\$\."
syn match smElse		contained	"\$|"
syn match smClauseCont	contained	"^\t"
syn region smClause	matchgroup=Delimiter start="\$?." matchgroup=Delimiter end="\$\." contains=smElse,smClause,smVar,smClauseCont

" The default highlighting.
hi def link smClause	Special
hi def link smClauseError	Error
hi def link smComment	Comment
hi def link smDefine	Statement
hi def link smElse	Delimiter
hi def link smHeader	Statement
hi def link smHeaderSep	String
hi def link smRewrite	Statement
hi def link smRewriteComment	Comment
hi def link smRewriteLhsToken	String
hi def link smRewriteLhsUser	Statement
hi def link smRewriteRhsToken	String
hi def link smRuleset	Statement
hi def link smVar		String

let b:current_syntax = "sm"

" vim: ts=18
