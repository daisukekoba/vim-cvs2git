" Vim syntax file
" Language:	crontab
" Maintainer:	John Hoelzel johnh51@bigfoot.com
" Last Change:	2000 Dec 10
" Filenames:    Linux: */crontab.*
" URL:		http://bigfoot.com/~johnh51/vim/syntax/crontab.vim
"
" line format:
" Minutes   Hours   Days   Months   Days_of_Week   Commands # comments

" Remove any old syntax stuff hanging around
syn clear
syn sync lines=2

" times
syn match crontabStatement	"^\s\{}\S\{1,}\s\{1,}\S\{1,}\s\{1,}\S\{1,}\s\{1,}\S\{1,}\s\{1,}\S\{1,}\s"

" comments
syn region crontabComment	start="#" end="$" 

" The default highlighting.
hi def link crontabStatement	Statement
hi def link crontabComment	Comment

let b:current_syntax = "crontab"

" vim: ts=8