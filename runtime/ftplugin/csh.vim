" Vim filetype plugin file
" Language:	csh
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Changed: 2002 Nov 26
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Csh:  thanks to Johannes Zellner
" - Both foreach and end must appear alone on separate lines.
" - The words else and endif must appear at the beginning of input lines;
"   the if must appear alone on its input line or after an else.
" - Each case label and the default label must appear at the start of a
"   line.
" - while and end must appear alone on their input lines.
if exists("loaded_matchit") && !exists("b:match_words")
    let b:match_words =
      \ '^\s*\<if\>.*(.*).*\<then\>:'.
      \   '^\s*\<else\>\s\+\<if\>.*(.*).*\<then\>:^\s*\<else\>:'.
      \   '^\s*\<endif\>,'.
      \ '\%(^\s*\<foreach\>\s\+\S\+\|^s*\<while\>\).*(.*):'.
      \   '\<break\>:\<continue\>:^\s*\<end\>,'.
      \ '^\s*\<switch\>.*(.*):^\s*\<case\>\s\+:^\s*\<default\>:^\s*\<endsw\>'
endif

" Change the :browse e filter to primarily show csh-related files.
if has("gui_win32") && !exists("b:browsefilter")
    let  b:browsefilter="csh Scripts (*.csh)\t*.csh\n" .
		\	"All Files (*.*)\t*.*\n"
endif
