"  matchit.vim: (global plugin) Extended "%" matching
"  Last Change: July 4, 2001
"  Maintainer:  Benji Fisher PhD   <benji@member.AMS.org>
"  Version:     1.0

" Documentation:
"  The documentation is in a separate file, matchit.txt .

" Credits:
"  Vim editor   by Bram Moolenaar (Thanks, Bram!)
"  Original script and design by Raul Segura Acevedo
"  Support for comments by Douglas Potts
"  Support for back references and other improvements by Benji Fisher
"  Support for many languages by Johannes Zellner
"  Suggestions for improvement, bug reports, and support for additional
"  languages by Jordi-Albert Batalla, Neil Bird, Mark Collett, Stephen Wall,
"  and Johannes Zellner.

" Debugging:
"  If you'd like to try the built-in debugging commands...
"   :MatchDebug      to activate debugging for the current buffer
"  This saves the values of several key script variables as buffer-local
"  variables.  See the MatchDebug() function, below, for details.

" TODO:  I should think about multi-line patterns for b:match_words.
" TODO:  Maybe I should add a menu so that people will actually use some of
" the features that I have implemented.

" allow user to prevent loading
" and prevent duplicate loading
if exists("loaded_matchit") || &cp
  finish
endif
let s:last_mps = ""
let loaded_matchit= 1
let s:last_words = ""

let s:save_cpo= &cpo
set cpo&vim

" The echo in the map clears the command line.
nnoremap% :<C-U>call <SID>Match_wrapper('',1,'n') \| echo<CR>
vnoremap% :<C-U>call <SID>Match_wrapper('',1,'v') \| echo<CR>m'gv``
nnoremap g% :<C-U>call <SID>Match_wrapper('',0,'n') \| echo<CR>
vnoremap g% :<C-U>call <SID>Match_wrapper('',0,'v') \| echo<CR>m'gv``

" Analogues of [{ and ]} using matching patterns:
nnoremap [% :<C-U>call <SID>MultiMatch("bW") <Bar> echo<CR>
vnoremap [% <Esc>[%m'gv``
vnoremap ]% <Esc>]%m'gv``
nnoremap ]% :<C-U>call <SID>MultiMatch("W") <Bar> echo<CR>

" Auto-complete mappings:  (not yet "ready for prime time")
" TODO Read :help write-plugin for the "right" way to let the user
" specify a key binding.
"   let g:match_auto = '<C-]>'
"   let g:match_autoCR = '<C-CR>'
" if exists("g:match_auto")
"   execute "inoremap " . g:match_auto . ' x<Esc>"=<SID>Autocomplete()<CR>Pls'
" endif
" if exists("g:match_autoCR")
"   execute "inoremap " . g:match_autoCR . ' <CR><C-R>=<SID>Autocomplete()<CR>'
" endif
" if exists("g:match_gthhoh")
"   execute "inoremap " . g:match_gthhoh . ' <C-O>:call <SID>Gthhoh()<CR>'
" endif " gthhoh = "Get the heck out of here!"

" Highlight group for error messages.  The user may choose to set this
" to be invisible.
hi link MatchError WarningMsg

let s:notslash = '\\\@<!\%(\\\\\)*'

function! s:Match_wrapper(word, forward, mode) range
  " If this function was called from Visual mode, make sure that the cursor
  " is at the correct end of the Visual range:
  if a:mode == "v"
    execute "normal! gv\<Esc>"
  endif
  " Use default behavior if called with a count or if no patterns are defined.
  if v:count
    exe "normal! " . v:count . "%"
    return
  elseif !exists("b:match_words") || b:match_words == ""
    silent! normal! %
    return
  end

  " First step:  if not already done, set the script variables
  "   s:do_BR	flag for whether there are backrefs
  "   s:pat	parsed version of b:match_words
  "   s:all	regexp based on s:pat and the default groups
" Thanks to Preben "Peppe" Guldberg and Bram Moolenaar for this suggestion!
  if (b:match_words != s:last_words) || (&mps != s:last_mps) ||
    \ exists("b:match_debug")
    let s:last_words = b:match_words
    let s:last_mps = &mps
    if b:match_words !~ s:notslash . '\\\d'
      let s:do_BR = 0
      let s:pat = b:match_words
    else
      let s:do_BR = 1
      let s:pat = s:ParseWords(b:match_words)
    endif
    " The next several lines were here before
    " BF started messing with this script.
    " quote the special chars in 'matchpairs', replace [,:] with \| and then
    " append the builtin pairs (/*, */, #if, #ifdef, #else, #elif, #endif)
    " let default = substitute(escape(&mps, '[$^.*~\\/?]'), '[,:]\+',
    "  \ '\\|', 'g').'\|\/\*\|\*\/\|#if\>\|#ifdef\>\|#else\>\|#elif\>\|#endif\>'
    let default = escape(&mps, '[$^.*~\\/?]') . (strlen(&mps) ? "," : "") .
      \ '\/\*:\*\/,#if\%(def\)\=:#else\>:#elif\>:#endif\>'
    " s:all = pattern with all the keywords
    let s:all = s:pat . (strlen(s:pat) ? "," : "") . default
    let s:all = substitute(s:all, s:notslash . '\zs[,:]\+', '\\|', 'g')
    let s:all = '\%(' . s:all . '\)'
    " let s:all = '\%(' . substitute(s:all, '\\\ze[,:]', '', 'g') . '\)'
    if exists("b:match_debug")
      let b:match_pat = s:pat
    endif
  endif

  " Second step:  set the following local variables:
  "     matchline = line on which the cursor started
  "     curcol    = number of characters before match
  "     prefix    = regexp for start of line to start of match
  "     suffix    = regexp for end of match to end of line
  " Require match to end on or after the cursor and prefer it to
  " start on or before the cursor.
  let matchline = getline(".")
  let startcol = col(".")
  if a:word != ''
    " word given
    if a:word !~ s:all
      echohl MatchError|echo 'Missing rule for word:"'.a:word.'"'|echohl NONE
      return
    endif
    let matchline = a:word
    let curcol = 0
    let prefix = '^\%('
    let suffix = '\)$'
  " Now the case when "word" is not given
  elseif matchend(matchline, '.*' . s:all) < startcol
    " there is no special word in this line || it ends before the cursor
    echohl MatchError | echo "No matching rule applies here" | echohl NONE
    return
  else	" Find the match that ends on or after the cursor and set curcol.
    let regexp = s:Wholematch(matchline, s:all, startcol-1)
    let curcol = match(matchline, regexp)
    let suf = strlen(matchline) - matchend(matchline, regexp)
    let prefix = (curcol ? '^.\{'  . curcol . '}\%(' : '^\%(')
    let suffix = (suf ? '\).\{' . suf . '}$'  : '\)$')
    " If the match comes from the defaults, bail out.
    if matchline !~ prefix .
      \ substitute(s:pat, s:notslash.'\zs[,:]\+', '\\|', 'g') . suffix
      norm! %
      return
    endif
  endif
  if exists("b:match_debug")
    let b:match_match = matchstr(matchline, regexp)
    let b:match_col = curcol+1
  endif

  " Third step:  Find the group and single word that match, and the original
  " (backref) versions of these.  Then, resolve the backrefs.
  " Set the following local variables:
  " group = colon-separated list of patterns, one of which matches
  "       = ini:mid:fin or ini:fin
  "
  " Reconstruct the version with unresolved backrefs.
  let patBR = substitute(b:match_words.',',
    \ s:notslash.'\zs[,:]*,[,:]*', ',', 'g')
  let patBR = substitute(patBR, s:notslash.'\zs:\{2,}', ':', 'g')
  " Now, set group and groupBR to the matching group: 'if:endif' or
  " 'while:endwhile' or whatever.
  let group = s:Choose(s:pat, matchline, ",", ":", prefix, suffix, patBR)
  let i = matchend(group, s:notslash . ",")
  let groupBR = strpart(group, i)
  let group = strpart(group, 0, i-1)
  " Now, matchline =~ prefix . substitute(group,':','\|','g') . suffix
  if s:do_BR " Do the hard part:  resolve those backrefs!
    let group = s:InsertRefs(groupBR, prefix, group, suffix, matchline)
  endif

  " Fourth step:  Set the arguments for searchpair().
  let i = matchend(group, s:notslash . ":")
  let j = matchend(group, '.*' . s:notslash . ":")
  let ini = strpart(group, 0, i-1)
  let mid = substitute(strpart(group, i,j-i-1), s:notslash.'\zs:', '\\|', 'g')
  let fin = strpart(group, j)
  " searchpair() requires that these patterns avoid \(\) groups.
  let ini = substitute(ini, s:notslash . '\zs\\(', '\\%(', 'g')
  let mid = substitute(mid, s:notslash . '\zs\\(', '\\%(', 'g')
  let fin = substitute(fin, s:notslash . '\zs\\(', '\\%(', 'g')
  " Set mid.  This is optimized for readability, not micro-efficiency!
  if a:forward && matchline =~ prefix . fin . suffix
    \ || !a:forward && matchline =~ prefix . ini . suffix
    let mid = ""
  endif
  " Set flag.  This is optimized for readability, not micro-efficiency!
  if a:forward && matchline =~ prefix . fin . suffix
    \ || !a:forward && matchline !~ prefix . ini . suffix
    let flag = "bW"
  else
    let flag = "W"
  endif
  " Set skip.
  if exists("b:match_skip")
    let skip = b:match_skip
  elseif exists("b:match_comment") " backwards compatibility and testing!
    let skip = "r:" . b:match_comment
  else
    let skip = 's:comment\|string'
  endif
  let skip = s:ParseSkip(skip)
  if exists("b:match_debug")
    let b:match_ini = ini
    let b:match_tail = (strlen(mid) ? mid.'\|' : '') . fin
  endif

  " Fifth step:  actually start moving the cursor and call searchpair().
  " This minimizes screen jumps and avoids using a global mark.
  let restore_cursor = line(".") . "G" . virtcol(".") . "|"
  normal! H
  let restore_cursor = "normal!" . line(".") . "Gzt" . restore_cursor
  execute restore_cursor
  " Later, :execute restore_cursor to get to the original screen.
  let restore_options = (&ic ? "" : "no") . "ignorecase"
  if exists("b:match_ignorecase")
    let &ignorecase = b:match_ignorecase
  endif
  " Instead of the next few lines, could I :execute "normal!".(curcol+1)."|" ?
  normal! 0
  if curcol
    execute "normal!" . curcol . "l"
  endif
  if skip =~ 'synID' && !(has("syntax") && exists("g:syntax_on"))
    let skip = "0"
  else
    execute "if " . skip . "| let skip = '0' | endif"
  endif
  let sp_return = searchpair(ini, mid, fin, flag, skip)
  let final_position = line(".") . "normal!" . virtcol(".") . "|"
  " Restore options, cursor position, and original screen.
  execute "set " . restore_options
  execute restore_cursor
  normal!m'
  if sp_return > 0
    execute final_position
  endif
endfun

" Example (simplified HTML patterns):  if
"   a:groupBR	= '<\(\k\+\)>:</\1>'
"   a:prefix	= '^.\{3}\('
"   a:group	= '<\(\k\+\)>:</\(\k\+\)>'
"   a:suffix	= '\).\{2}$'
"   a:matchline	=  "123<tag>12" or "123</tag>12"
" then extract "tag" from a:matchline and return "<tag>:</tag>" .
fun! s:InsertRefs(groupBR, prefix, group, suffix, matchline)
  if a:matchline !~ a:prefix .
    \ substitute(a:group, s:notslash . '\zs:', '\\|', 'g') . a:suffix
    return a:group
  endif
  let i = matchend(a:groupBR, s:notslash . ':')
  let ini = strpart(a:groupBR, 0, i-1)
  let tailBR = strpart(a:groupBR, i)
  let word = s:Choose(a:group, a:matchline, ":", "", a:prefix, a:suffix,
    \ a:groupBR)
  let i = matchend(word, s:notslash . ":")
  let wordBR = strpart(word, i)
  let word = strpart(word, 0, i-1)
  " Now, a:matchline =~ a:prefix . word . a:suffix
  if wordBR != ini
    let table = s:Resolve(ini, wordBR, "table")
  else
    " let table = "----------"
    let table = ""
    let d = 0
    while d < 10
      if tailBR =~ s:notslash . '\\' . d
	" let table[d] = d
	let table = table . d
      else
	let table = table . "-"
      endif
      let d = d + 1
    endwhile
  endif
  let d = 9
  while d
    if table[d] != "-"
      let backref = substitute(a:matchline, a:prefix.word.a:suffix,
	\ '\'.table[d], "")
	" Are there any other characters that should be escaped?
      let backref = escape(backref, '*,:')
      execute s:Ref(ini, d, "start", "len")
      let ini = strpart(ini, 0, start) . backref . strpart(ini, start+len)
      let tailBR = substitute(tailBR, s:notslash . '\zs\\' . d,
	\ escape(backref, '\\'), 'g')
    endif
    let d = d-1
  endwhile
  if exists("b:match_debug")
    if s:do_BR
      let b:match_table = table
      let b:match_word = word
    else
      let b:match_table = ""
      let b:match_word = ""
    endif
  endif
  return ini . ":" . tailBR
endfun

" Input a comma-separated list of groups with backrefs, such as
"   a:groups = '\(foo\):end\1,\(bar\):end\1'
" and return a comma-separated list of groups with backrefs replaced:
"   return '\(foo\):end\(foo\),\(bar\):end\(bar\)'
fun! s:ParseWords(groups)
  let groups = substitute(a:groups.",", s:notslash.'\zs[,:]*,[,:]*', ',', 'g')
  let groups = substitute(groups, s:notslash . '\zs:\{2,}', ':', 'g')
  let parsed = ""
  while groups =~ '[^,:]'
    let i = matchend(groups, s:notslash . ':')
    let j = matchend(groups, s:notslash . ',')
    let ini = strpart(groups, 0, i-1)
    let tail = strpart(groups, i, j-i-1) . ":"
    let groups = strpart(groups, j)
    let parsed = parsed . ini
    let i = matchend(tail, s:notslash . ':')
    while i != -1
      " In 'if:else:endif', ini='if' and word='else' and then word='endif'.
      let word = strpart(tail, 0, i-1)
      let tail = strpart(tail, i)
      let i = matchend(tail, s:notslash . ':')
      let parsed = parsed . ":" . s:Resolve(ini, word, "word")
    endwhile " Now, tail has been used up.
    let parsed = parsed . ","
  endwhile " groups =~ '[^,:]'
  return parsed
endfun

" TODO I think this can be simplified and/or made more efficient.
" TODO What should I do if a:start is out of range?
" Return a regexp that matches all of a:string, such that
" matchstr(a:string, regexp) represents the match for a:pat that starts
" as close to a:start as possible, before being preferred to after, and
" ends after a:start .
" Usage:
" let regexp = s:Wholematch(getline("."), 'foo\|bar', col(".")-1)
" let i      = match(getline("."), regexp)
" let j      = matchend(getline("."), regexp)
" let match  = matchstr(getline("."), regexp)
fun! s:Wholematch(string, pat, start)
  if a:pat =~ '^\\%\=(.*\\)$'
    let group = a:pat
  else
    let group = '\(' . a:pat . '\)'
  endif
  let prefix = (a:start ? '\(^.\{,' . a:start . '}\)\@<=' : '^')
  let len = strlen(a:string)
  let suffix = (a:start+1 < len ? '\(.\{,'.(len-a:start-1).'}$\)\@=' : '$')
  if a:string !~ prefix . group . suffix
    let prefix = ''
  endif
  return prefix . group . suffix
endfun

" No extra arguments:  s:Ref(string, d) will
" find the d'th occurrence of '\(' and return it, along with everything up
" to and including the matching '\)'.
" One argument:  s:Ref(string, d, "start") returns the index of the start
" of the d'th '\(' and any other argument returns the length of the group.
" Two arguments:  s:Ref(string, d, "foo", "bar") returns a string to be
" executed, having the effect of
"   :let foo = s:Ref(string, d, "start")
"   :let bar = s:Ref(string, d, "len")
fun! s:Ref(string, d, ...)
  let len = strlen(a:string)
  if a:d == 0
    let start = 0
  else
    let cnt = a:d
    let match = a:string
    while cnt
      let cnt = cnt - 1
      let index = matchend(match, s:notslash . '\\(')
      if index == -1
	return ""
      endif
      let match = strpart(match, index)
    endwhile
    let start = len - strlen(match)
    if a:0 == 1 && a:1 == "start"
      return start - 2
    endif
    let cnt = 1
    while cnt
      let index = matchend(match, s:notslash . '\\(\|\\)') - 1
      if index == -2
	return ""
      endif
      " Increment if an open, decrement if a ')':
      let cnt = cnt + (match[index]=="(" ? 1 : -1)  " ')'
      " let cnt = stridx('0(', match[index]) + cnt
      let match = strpart(match, index+1)
    endwhile
    let start = start - 2
    let len = len - start - strlen(match)
  endif
  if a:0 == 1
    return len
  elseif a:0 == 2
    return "let " . a:1 . "=" . start . "| let " . a:2 . "=" . len
  else
    return strpart(a:string, start, len)
  endif
endfun

" Count the number of disjoint copies of pattern in string.
" If the pattern is a literal string and contains no '0' or '1' characters
" then s:Count(string, pattern, '0', '1') should be faster than
" s:Count(string, pattern).
fun! s:Count(string, pattern, ...)
  let pat = escape(a:pattern, '\\')
  if a:0 > 1
    let foo = substitute(a:string, '[^'.a:pattern.']', "a:1", "g")
    let foo = substitute(a:string, pat, a:2, "g")
    let foo = substitute(foo, '[^' . a:2 . ']', "", "g")
    return strlen(foo)
  endif
  let result = 0
  let foo = a:string
  let index = matchend(foo, pat)
  while index != -1
    let result = result + 1
    let foo = strpart(foo, index)
    let index = matchend(foo, pat)
  endwhile
  return result
endfun

" s:Resolve('\(a\)\(b\)', '\(c\)\2\1\1\2') should return table.word, where
" word = '\(c\)\(b\)\(a\)\3\2' and table = '-32-------'.  That is, the first
" '\1' in target is replaced by '\(a\)' in word, table[1] = 3, and this
" indicates that all other instances of '\1' in target are to be replaced
" by '\3'.  The hard part is dealing with nesting...
" Note that ":" is an illegal character for source and target,
" unless it is preceded by "\".
fun! s:Resolve(source, target, output)
  let word = a:target
  let i = matchend(word, s:notslash . '\\\d') - 1
  let table = "----------"
  while i != -2 " There are back references to be replaced.
    let d = word[i]
    let backref = s:Ref(a:source, d)
    " The idea is to replace '\d' with backref.  Before we do this,
    " replace any \(\) groups in backref with :1, :2, ... if they
    " correspond to the first, second, ... group already inserted
    " into backref.  Later, replace :1 with \1 and so on.  The group
    " number w+b within backref corresponds to the group number
    " s within a:source.
    " w = number of '\(' in word before the current one
    let w = s:Count(
    \ substitute(strpart(word, 0, i-1), '\\\\', '', 'g'), '\(', '1')
    let b = 1 " number of the current '\(' in backref
    let s = d " number of the current '\(' in a:source
    while b <= s:Count(substitute(backref, '\\\\', '', 'g'), '\(', '1')
    \ && s < 10
      if table[s] == "-"
	if w + b < 10
	  " let table[s] = w + b
	  let table = strpart(table, 0, s) . (w+b) . strpart(table, s+1)
	endif
	let b = b + 1
	let s = s + 1
      else
	execute s:Ref(backref, b, "start", "len")
	let ref = strpart(backref, start, len)
	let backref = strpart(backref, 0, start) . ":". table[s]
	\ . strpart(backref, start+len)
	let s = s + s:Count(substitute(ref, '\\\\', '', 'g'), '\(', '1')
      endif
    endwhile
    let word = strpart(word, 0, i-1) . backref . strpart(word, i+1)
    let i = matchend(word, s:notslash . '\\\d') - 1
  endwhile
  let word = substitute(word, s:notslash . '\zs:', '\\', 'g')
  if a:output == "table"
    return table
  elseif a:output == "word"
    return word
  else
    return table . word
  endif
endfun

" Assume a:comma = ",".  Then the format for a:patterns and a:1 is
"   a:patterns = "<pat1>,<pat2>,..."
"   a:1 = "<alt1>,<alt2>,..."
" If <patn> is the first pattern that matches a:string then return <patn>
" if no optional arguments are given; return <patn>,<altn> if a:1 is given.
fun! s:Choose(patterns, string, comma, branch, prefix, suffix, ...)
  let tail = (a:patterns =~ a:comma."$" ? a:patterns : a:patterns . a:comma)
  let i = matchend(tail, s:notslash . a:comma)
  if a:0
    let alttail = (a:1 =~ a:comma."$" ? a:1 : a:1 . a:comma)
    let j = matchend(alttail, s:notslash . a:comma)
  endif
  let current = strpart(tail, 0, i-1)
  if a:branch == ""
    let currpat = current
  else
    let currpat = substitute(current, a:branch, '\\|', 'g')
  endif
  while a:string !~ a:prefix . currpat . a:suffix
    let tail = strpart(tail, i)
    let i = matchend(tail, s:notslash . a:comma)
    if i == -1
      return -1
    endif
    let current = strpart(tail, 0, i-1)
    if a:branch == ""
      let currpat = current
    else
      let currpat = substitute(current, a:branch, '\\|', 'g')
    endif
    if a:0
      let alttail = strpart(alttail, j)
      let j = matchend(alttail, s:notslash . a:comma)
    endif
  endwhile
  if a:0
    let current = current . a:comma . strpart(alttail, 0, j-1)
  endif
  return current
endfun

" Call this function to turn on debugging information.  Every time the main
" script is run, buffer variables will be saved.  These can be used directly
" or viewed using the menu items below.
if !exists(":MatchDebug")
  command! -nargs=0 MatchDebug call s:Match_debug()
endif

fun! s:Match_debug()
  let b:match_debug = 1	" Save debugging information.
  " pat = all of b:match_words with backrefs parsed
  amenu &Matchit.&pat	:echo b:match_pat<CR>
  " match = bit of text that is recognized as a match
  amenu &Matchit.&match	:echo b:match_match<CR>
  " curcol = cursor column of the start of the matching text
  amenu &Matchit.&curcol	:echo b:match_col<CR>
  " wholeBR = matching group, original version
  amenu &Matchit.wh&oleBR	:echo b:match_wholeBR<CR>
  " iniBR = 'if' piece, original version
  amenu &Matchit.ini&BR	:echo b:match_iniBR<CR>
  " ini = 'if' piece, with all backrefs resolved from match
  amenu &Matchit.&ini	:echo b:match_ini<CR>
  " tail = 'else\|endif' piece, with all backrefs resolved from match
  amenu &Matchit.&tail	:echo b:match_tail<CR>
  " fin = 'endif' piece, with all backrefs resolved from match
  amenu &Matchit.&word	:echo b:match_word<CR>
  " '\'.d in ini refers to the same thing as '\'.table[d] in word.
  amenu &Matchit.t&able	:echo '0:' . b:match_table . ':9'<CR>
endfun

" Jump to the nearest unmatched "(" or "if" or "<tag>" if a:spflag == "bW"
" or the nearest unmatched "</tag>" or "endif" or ")" if a:spflag == "W".
" Return a "mark" for the original position, so that
"   let m = MultiMatch("bW") ... execute m
" will return to the original position.  If there is a problem, do not
" move the cursor and return "", unless a count is given, in which case
" go up or down as many levels as possible and again return "".
" TODO This relies on the same patterns as % matching.  It might be a good
" idea to give it its own matching patterns.
fun! s:MultiMatch(spflag)
  if !exists("b:match_words") || b:match_words == ""
    return ""
  end

  " First step:  if not already done, set the script variables
  "   s:do_BR	flag for whether there are backrefs
  "   s:pat	parsed version of b:match_words
  "   s:all	regexp based on s:pat and the default groups
  " This part is copied and slightly modified from s:Match_wrapper().
  let default = escape(&mps, '[$^.*~\\/?]') . (strlen(&mps) ? "," : "") .
    \ '\/\*:\*\/,#if\%(def\)\=:$else\>:#elif\>:#endif\>'
  if (b:match_words != s:last_words) || (&mps != s:last_mps) ||
    \ exists("b:match_debug")
    let s:last_words = b:match_words
    let s:last_mps = &mps
    if b:match_words !~ s:notslash . '\\\d'
      let s:do_BR = 0
      let s:pat = b:match_words
    else
      let s:do_BR = 1
      let s:pat = s:ParseWords(b:match_words)
    endif
    let s:all = '\%(' . substitute(s:pat . (strlen(s:pat)?",":"") . default,
      \	'[,:]\+','\\|','g') . '\)'
    if exists("b:match_debug")
      let b:match_pat = s:pat
    endif
  endif

  " Second step:  figure out the patterns for searchpair()
  " and save the screen, cursor position, and 'ignorecase'.
  " - TODO:  A lot of this is copied from s:Match_wrapper().
  " - maybe even more functionality should be split off
  " - into separate functions!
  let open =  substitute(s:pat . default, ':[^,]*,', '\\),\\(', 'g')
  let open =  '\(' . substitute(open, ':[^,]*$', '\\)', '')
  let close = substitute(s:pat . default, ',[^,]*:', '\\),\\(', 'g')
  let close = substitute(close, '[^,]*:', '\\(', '') . '\)'
  if exists("b:match_skip")
    let skip = b:match_skip
  elseif exists("b:match_comment") " backwards compatibility and testing!
    let skip = "r:" . b:match_comment
  else
    let skip = 's:comment\|string'
  endif
  let skip = s:ParseSkip(skip)
  let restore_cursor = line(".") . "G" . virtcol(".") . "|"
  normal! H
  let restore_cursor = "normal!" . line(".") . "Gzt" . restore_cursor
  execute restore_cursor
  let restore_options = (&ic ? "" : "no") . "ignorecase"
  if exists("b:match_ignorecase")
    let &ignorecase = b:match_ignorecase
  endif

  " Third step: call searchpair().
  " Replace '\('--but not '\\('--with '\%(' and ',' with '\|'.
  let openpat =  substitute(open, '\(\\\@<!\(\\\\\)*\)\@<=\\(', '\\%(', 'g')
  let openpat = substitute(openpat, ',', '\\|', 'g')
  let closepat = substitute(close, '\(\\\@<!\(\\\\\)*\)\@<=\\(', '\\%(', 'g')
  let closepat = substitute(closepat, ',', '\\|', 'g')
  if skip =~ 'synID' && !(has("syntax") && exists("g:syntax_on"))
    let skip = '0'
  else
    execute "if " . skip . "| let skip = '0' | endif"
  endif
  mark '
  let level = v:count1
  while level
    if searchpair(openpat, '', closepat, a:spflag, skip) < 1
      execute "set " . restore_options
      return ""
    endif
    let level = level - 1
  endwhile

  " Restore options and return a string to restore the original position.
  execute "set " . restore_options
  return restore_cursor
endfun

" Search backwards for "if" or "while" or "<tag>" or ...
" and return "endif" or "endwhile" or "</tag>" or ... .
" For now, this uses b:match_words and the same script variables
" as s:Match_wrapper() .  Later, it may get its own patterns,
" either from a buffer variable or passed as arguments.
" fun! s:Autocomplete()
"   echo "autocomplete not yet implemented :-("
"   if !exists("b:match_words") || b:match_words == ""
"     return ""
"   end
"   let startpos = s:MultiMatch("bW")
"
"   if startpos == ""
"     return ""
"   endif
"   " - TODO:  figure out whether 'if' or '<tag>' matched, and construct
"   " - the appropriate closing.
"   let matchline = getline(".")
"   let curcol = col(".") - 1
"   " - TODO:  Change the s:all argument if there is a new set of match pats.
"   let regexp = s:Wholematch(matchline, s:all, curcol)
"   let suf = strlen(matchline) - matchend(matchline, regexp)
"   let prefix = (curcol ? '^.\{'  . curcol . '}\%(' : '^\%(')
"   let suffix = (suf ? '\).\{' . suf . '}$'  : '\)$')
"   " Reconstruct the version with unresolved backrefs.
"   let patBR = substitute(b:match_words.',', '[,:]*,[,:]*', ',', 'g')
"   let patBR = substitute(patBR, ':\{2,}', ':', "g")
"   " Now, set group and groupBR to the matching group: 'if:endif' or
"   " 'while:endwhile' or whatever.
"   let group = s:Choose(s:pat, matchline, ",", ":", prefix, suffix, patBR)
"   let i = matchend(group, s:notslash . ",")
"   let groupBR = strpart(group, i)
"   let group = strpart(group, 0, i-1)
"   " Now, matchline =~ prefix . substitute(group,':','\|','g') . suffix
"   if s:do_BR
"     let group = s:InsertRefs(groupBR, prefix, group, suffix, matchline)
"   endif
" " let g:group = group
"
"   " - TODO:  Construct the closing from group.
"   let fake = "end" . expand("<cword>")
"   execute startpos
"   return fake
" endfun

" Close all open structures.  "Get the heck out of here!"
" fun! s:Gthhoh()
"   let close = s:Autocomplete()
"   while strlen(close)
"     put=close
"     let close = s:Autocomplete()
"   endwhile
" endfun

" Parse special strings as typical skip arguments for searchpair():
"   s:foo becomes (current syntax item) =~ foo
"   S:foo becomes (current syntax item) !~ foo
"   r:foo becomes (line before cursor) =~ foo
"   R:foo becomes (line before cursor) !~ foo
fun! s:ParseSkip(str)
  let skip = a:str
  if skip[1] == ":"
    if skip[0] == "s"
      let skip = "synIDattr(synID(line('.'),col('.'),1),'name') =~? '" .
	\ strpart(skip,2) . "'"
    elseif skip[0] == "S"
      let skip = "synIDattr(synID(line('.'),col('.'),1),'name') !~? '" .
	\ strpart(skip,2) . "'"
    elseif skip[0] == "r"
      let skip = "strpart(getline('.'),0,col('.'))=~'" . strpart(skip,2). "'"
    elseif skip[0] == "R"
      let skip = "strpart(getline('.'),0,col('.'))!~'" . strpart(skip,2). "'"
    endif
  endif
  return skip
endfun

aug Matchit
  au!
  " Ada:  thanks to Neil Bird.
  let s:notend = '\%(\<end\s\+\)\@<!'
  au FileType ada if !exists("b:match_words") |
    \ let b:match_words=
  \ s:notend . '\<if\>:\<elsif\>:\<else\>:\<end\>\s\+\<if\>,' .
  \ s:notend . '\<case\>:\<when\>:\<end\>\s\+\<case\>,' .
  \ '\%(\<while\>.*\|\<for\>.*\|'.s:notend.'\)\<loop\>:\<end\>\s\+\<loop\>,' .
  \ '\%(\<do\>\|\<begin\>\):\<exception\>:\<end\>\s*\%($\|[;A-Z]\),' .
  \ s:notend . '\<record\>:\<end\>\s\+\<record\>'
  \ | endif
  " ASP:  Active Server Pages (with Visual Basic Script)
  " thanks to Gontran BAERTS
  au FileType aspvbs if !exists("b:match_words") |
    \ do Matchit FileType html |
  \ let b:match_words =
  \ s:notend . '\<If\>:^\s\+\<Else\>:\<ElseIf\>:\<end\s\+\<if\>,' .
  \ s:notend . '\<Select\s\+\<Case\>:\<Case\>:\<Case\s\+\<Else\>:' .
  \	'\<End\s\+\<Select\>,' .
  \ '^\s*\<Sub\>:\<End\s\+\<Sub\>,' .
  \ '^\s*\<Function\>:\<End\s\+\<Function\>,' .
  \ '\<Class\>:\<End\s\+\<Class\>,' .
  \ '^\s*\<Do\>:\<Loop\>,' .
  \ '^\s*\<For\>:\<Next\>,' .
  \ '\<While\>:\<Wend\>,' .
  \ b:match_words
  \ | endif
  " Csh:  thanks to Johannes Zellner
  " - Both foreach and end must appear alone on separate lines.
  " - The words else and endif must appear at the beginning of input lines;
  "   the if must appear alone on its input line or after an else.
  " - Each case label and the default label must appear at the start of a
  "   line.
  " - while and end must appear alone on their input lines.
  au FileType csh,tcsh  if !exists("b:match_words") |
    \ let b:match_words =
      \ '^\s*\<if\>.*(.*).*\<then\>:'.
      \   '^\s*\<else\>\s\+\<if\>.*(.*).*\<then\>:^\s*\<else\>:'.
      \   '^\s*\<endif\>,'.
      \ '\%(^\s*\<foreach\>\s\+\S\+\|^s*\<while\>\).*(.*):'.
      \   '\<break\>:\<continue\>:^\s*\<end\>,'.
      \ '^\s*\<switch\>.*(.*):^\s*\<case\>\s\+:^\s*\<default\>:^\s*\<endsw\>'
      \ | endif
  " DTD:  thanks to Johannes Zellner
  " - match <!--, --> style comments.
  " - match <! with >
  " - TODO:  why does '--:--,'. not work ?
  au! FileType dtd if !exists("b:match_words") |
    \ let b:match_words =
    \ '<!--:-->,'.
    \ '<!:>'
    \ | endif
  " Entity:  see XML.
  " Essbase:
  au BufNewFile,BufRead *.csc if !exists("b:match_words") |
    \ let b:match_words=
  \ '\<fix\>:\<endfix\>,' .
  \ '\<if\>:\<else\%(if\)\=\>:\<endif\>,' .
  \ '\<!loopondimensions\>\|\<!looponselected\>:\<!endloop\>'
  \ | endif
  " Fortran:  thanks to Johannes Zellner and Ajit Thakkar (77, 90, 95)
  let s:notelse = '\%(\<end\s\+\|\<else\s\+\)\@<!'
  au FileType fortran if !exists("b:match_words") |
    \ let b:match_ignorecase = 1 |
    \ let b:match_words =
    \ '\<select\s*case\>:\<case\>:\<end\s*select\>,' .
    \ s:notelse . '\<if\s*(.\+)\s*then\>:' .
      \ '\<else\s*\%(if\s*(.\+)\s*then\)\=\>:\<end\s*if\>,'.
    \ 'do\s\+\(\d\+\):\(^\s*\)\@<=\1\s,'.
    \ s:notend . '\<do\>:\<end\s*do\>,'.
    \ s:notelse . '\<where\>:\<elsewhere\>:\<end\s*where\>,'.
    \ s:notend . '\<type\s*[^(]:\<end\s*type\>,'.
    \ s:notend . '\<subroutine\>:\<end\s*subroutine\>,'.
    \ s:notend . '\<function\>:\<end\s*function\>,'.
    \ s:notend . '\<module\>:\<end\s*module\>,'.
    \ s:notend . '\<program\>:\<end\s*program\>'
    \ | endif
  " HTML:  thanks to Johannes Zellner.
    au FileType html,jsp if !exists("b:match_words") |
      \	let b:match_ignorecase = 1 |
      \	let b:match_skip = 's:Comment' |
      \ let b:match_words = '<:>,' .
      \ '<\@<=[ou]l[^>]*\%(>\|$\):<\@<=li>:<\@<=/[ou]l>,' .
      \ '<\@<=\([^/][^ \t>]*\)[^>]*\%(>\|$\):<\@<=/\1>'
      \ | endif
  " LaTeX:
  au FileType tex if !exists("b:match_words") |
    \ let b:match_ignorecase = 0
    \ | let b:match_skip = 'r:\\\@<!\%(\\\\\)*%'
    \ | let b:match_words = '(:),\[:],{:},\\(:\\),\\\[:\\],' .
    \ '\\begin\s*\({\a\+\*\=}\):\\end\s*\1'
    \ | endif
  " Lua version 4.0+:  thanks to Max Ischenko
  au FileType lua if !exists("b:match_words") |
    \ let b:match_ignorecase = 0 | let b:match_words =
    \ '\<\%(do\|function\|if\)\>:' .
    \ '\<\%(return\|else\|elseif\)\>:' .
    \ '\<end\>,' .
    \ '\<repeat\>:\<until\>'
    \ | endif
  " Pascal:
  au FileType pascal if !exists("b:match_words") |
    \ let b:match_words='\<begin\>:\<end\>'
    \ | endif
  " SGML:  see XML
  " Shell:  thanks to Johannes Zellner
  let s:sol = '\%(;\s*\|^\s*\)\@<='  " start of line
  au FileType sh,config if !exists("b:match_words") |
    \ let b:match_words =
      \ s:sol.'if\>:' . s:sol.'elif\>:' . s:sol.'else\>:' . s:sol. 'fi\>,' .
      \ s:sol.'\%(for\|while\)\>:' . s:sol. 'done\>,' .
      \ s:sol.'case\>:' . s:sol. 'esac\>'
      \ | endif
  " RPM Spec:  thanks to Max Ischenko
  au FileType spec if !exists("b:match_words") |
    \ let b:match_ignorecase = 0 | let b:match_words =
    \ '^Name:^%description:^%clean:^%setup:^%build:^%install:^%files:' .
    \ '^%package:^%preun:^%postun:^%changelog'
    \ | endif
  " Tcsh:  see Csh
  " TeX:  see LaTeX; I do not think plain TeX needs this.
  " Verilog:  thanks to Mark Collett
  au FileType verilog if !exists("b:match_words") |
    \ let b:match_ignorecase = 0
      \ | let b:match_words =
      \ '\<begin\>:\<end\>,'.
      \ '\<case\>\|\<casex\>\|\<casez\>:\<endcase\>,'.
      \ '\<module\>:\<endmodule\>,'.
      \ '\<function\>:\<endfunction\>,'.
      \ '`ifdef\>:`else\>:`endif\>'
      \ | endif
  " Vim:
  au FileType vim if !exists("b:match_words") |
    \ let b:match_ignorecase=0 |
  \ let b:match_strings_like_comments = 1 |
  \ let b:match_words=
  \ '\<fun\%[ction]\>:\<retu\%[rn]\>:\<endf\%[unction]\>,' .
  \ '\<while\>:\<break\>:\<con\%[tinue]\>:\<endw\%[hile]\>,' .
  \ '\<if\>:\<el\%[seif]\>:\<en\%[dif]\>,' .
  \ '\<aug\%[roup]\s\+\%(END\>\)\@!\S:\<aug\%[roup]\s\+END\>,' .
  \ '(:)'
  \ | endif
  " XML:  thanks to Johannes Zellner and Akbar Ibrahim
  " - case sensitive
  " - don't match empty tags <fred/>
  " - match <!--, --> style comments (but not --, --)
  " - match <!, > inlined dtd's. This is not perfect, as it
  "   gets confused for example by
  "       <!ENTITY gt ">">
  au! FileType xml,sgml,entity if !exists("b:match_words") |
    \ let b:match_ignorecase=0 | let b:match_words =
    \ '<:>,' .
    \ '<\@<=!\[CDATA\[:]]>,'.
    \ '<\@<=!--:-->,'.
    \ '<\@<=?\k\+:?>,'.
    \ '<\@<=\([^ \t>/]\+\)\%(\s\+[^>]*\%([^/]>\|$\)\|>\|$\):<\@<=/\1>,'.
    \ '<\@<=\%([^ \t>/]\+\)\%(\s\+[^/>]*\|$\):/>'
    \ | endif

aug END

let &cpo= s:save_cpo

" vim:sts=2:sw=2:
