" Vim indent file
" Language:	Shell script
" Maintainer:	Nikolai Weibull <da.box@home.se>
" Last Change:	2001 Jun 20

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetShIndent()
setlocal indentkeys+==else,=elif,=esac,=fi,=done indentkeys-=0#

" Only define the function once.
if exists("*GetShIndent")
  finish
endif

function GetShIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " Hit the start of the file, use zero indent.
  if lnum == 0
    return 0
  endif

  " Add a 'shiftwidth' after if, while, else, case, until, for, function()
  let ind = indent(lnum)
  let line = getline(lnum)
  if line =~ '^\s*\(if\|else\|elif\|case\|while\|until\|for\)\>'
      \ || line =~ '^\s*\<\h\w*\>\s*()\s*{'
      \ || line =~ '^\s*{'
    if line !~ '\(esac\|fi\|done\)\>\s*$' && line !~ '}\s*$'
      let ind = ind + &sw
    endif
  endif

  " Subtract a 'shiftwidth' on a else, esac, fi, done
  let line = getline(v:lnum)
  if line =~ '^\s*\(else\|esac\|fi\|done\)\>' || line =~ '^\s*}'
    let ind = ind - &sw
  endif

  return ind
endfunction

" vim: set sw=2 :