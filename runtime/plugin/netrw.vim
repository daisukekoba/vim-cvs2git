" netrw.vim: (global plugin) Handles file transfer across a network
"  Last Change: May 30, 2001
"  Maintainer:  Charles E. Campbell, Jr. PhD   <cec@NgrOyphSon.gPsfAc.nMasa.gov>
"  Version:     2.11

" Credits:
"  Vim editor   by Bram Moolenaar (Thanks, Bram!)
"  rcp, ftp support by C Campbell <cec@NgrOyphSon.gPsfAc.nMasa.gov>
"  scp  support by raf            <raf@comdyn.com.au>
"  http support by Bram Moolenaar <bram@moolenaar.net>
"  inputsecret(), BufReadCmd, BufWriteCmd contributed by C Campbell

" Debugging:
"  If you'd like to try the built-in debugging commands...
"   :g/DBG/s/^"//      to activate    debugging
"   :g/DBG/s/^/"/      to de-activate debugging
"  You'll need to get <Decho.vim> and put it into your <.vim/plugin>
"  (or <vimfiles\plugin> for Windows).  Its available at
"  http://www.erols.com/astronaut/vim/vimscript/Decho.vim

" Option:
"   let g:netrw_ftp = 1
"      If you're having trouble with ftp-.netrc file (ie. you don't
"      have a <.netrc> file) then you may wish to try putting the
"      statement above in your <.vimrc> file.

" Reading:
" :Nread ?                             give help
" :Nread machine:file                  uses rcp
" :Nread "machine file"                uses ftp with <.netrc>
" :Nread "machine id password file"    uses ftp
" :Nread "ftp://machine[#port]/file"   uses ftp  (autodetects <.netrc>)
" :Nread "http://[user@]machine/file"  uses http (wget)
" :Nread "rcp://machine/file"          uses rcp
" :Nread "scp://[user@]machine/file"   uses scp

" Writing:
" :Nwrite ?                             give help
" :Nwrite machine:file                  uses rcp
" :Nwrite "machine file"                uses ftp with <.netrc>
" :Nwrite "machine id password file"    uses ftp
" :Nwrite "ftp://machine[#port]/file"   uses ftp  (autodetects <.netrc>)
" :Nwrite "rcp://machine/file"          uses rcp
" :Nwrite "scp://[user@]machine/file"   uses scp
" http: not supported!

" User And Password Changing:
"  Attempts to use ftp will prompt you for a user-id and a password.
"  These will be saved in g:netrw_uid and g:netrw_passwd
"  Subsequent uses of ftp will re-use those.  If you need to use
"  a different user id and/or password, you'll want to
"  call NetUserPass() first.

"   :NetUserPass [uid [password]]       -- will prompt as needed
"   :call NetUserPass()                 -- will prompt for userid and password
"   :call NetUserPass("uid")            -- will prompt for password
"   :call NetUserPass("uid","password") -- sets global userid and password

" Variables:
"    b:netrw_lastfile : last file Network-read/written retained on
"                       a per-buffer basis (supports bare :Nw )
"    b:netrw_line     : during Nw/NetWrite, holds current line   number
"    b:netrw_col      : during Nw/NetWrite, holds current column number
"                       b:netrw_line and b:netrw_col are used to restore
"                       the cursor position on writes
"
"    g:netrw_uid      : (ftp) user id,      retained on a per-session basis
"    g:netrw_passwd   : (ftp) password,     retained on a per-session basis
"    g:netrw_ftp      : if it doesn't exist, use default ftp (user id pass)
"                       =1 : use alternate ftp method

"  But be doers of the word, and not only hearers, deluding your own selves
"  (James1:22 RSV)
" =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

" Exit quickly when already loaded or when 'compatible' is set.
if exists("loaded_netrw") || &cp
  finish
endif
let loaded_netrw = 1
let s:save_cpo   = &cpo
set cpo&vim

" Vimrc Support:
" Auto-detection for ftp://*, rcp://*, scp://*, and http://*
" Should make file transfers across networks transparent.  Currently I haven't
" supported appends.  Hey, gotta leave something for <netrw.vim> version 3!
if version >= 600
  augroup Network
    au!
    au BufReadCmd	ftp://*,rcp://*,scp://*,http://* exe "Nread 0r " . expand("<afile>") | exe "doau BufReadPost " . expand("<afile>")
    au FileReadCmd	ftp://*,rcp://*,scp://*,http://* exe "Nread "    . expand("<afile>") | exe "doau BufReadPost " . expand("<afile>")
    au BufWriteCmd	ftp://*,rcp://*,scp://* exe "Nwrite "    . expand("<afile>")
  augroup END
endif

" ------------------------------------------------------------------------

" Commands: :Nread, :Nwrite, and :NetUserPass
:com -nargs=* Nread call s:NetRead(<f-args>)
:com -range=% -nargs=* Nwrite let b:netrw_line=line(".")|let b:netrw_col=col(".")-1 | <line1>,<line2>call s:NetWrite(<f-args>)
:com -nargs=* NetUserPass call NetUserPass(<f-args>)

" ------------------------------------------------------------------------

" NetRead: responsible for reading a file over the net
function! s:NetRead(...)
" Decho "DBG: NetRead(a:1<".a:1.">) {"

 " Get Temporary Filename
 let cinkeep=&cin
 let aikeep=&ai
 let cinokeep=&cino
 let comkeep=&com
 let cpokeep=&cpo
 set nocin noai
 set cino=
 set com=
 set cpo-=aA

 " get temporary file
 let tmpfile = tempname()

 " Special Exception: if a file is named "0r", then
 "                    "0r" will be used to read the
 "                    following files instead of "r"
 if     a:0 == 0
  let readcmd= "r"
  let ichoice= 0
 elseif a:1 == "0r"
  let readcmd = "0r"
  let ichoice = 2
 else
  let readcmd = "r"
  let ichoice = 1
 endif

 while ichoice <= a:0

  " attempt to repeat with previous host-file-etc
  if exists("b:netrw_lastfile") && a:0 == 0
"   Decho "DBG: using b:netrw_lastfile<" . b:netrw_lastfile . ">"
   let choice = b:netrw_lastfile
   let ichoice= ichoice + 1
  else
   exe "let choice= a:" . ichoice
"   Decho "DBG: NetRead1: choice<" . choice . ">"

   " Reconstruct Choice if choice starts with '"'
   if match(choice,"?") == 0
    echo "NetRead Usage:"
    echo ":Nread machine:file                  uses rcp"
    echo ":Nread \"machine file\"                uses ftp with <.netrc>"
    echo ":Nread \"machine id password file\"    uses ftp"
    echo ":Nread ftp://machine[#port]/file       uses ftp  (autodetects <.netrc>)"
    echo ":Nread http://[user@]machine/file    uses http (wget)"
    echo ":Nread rcp://machine/file            uses rcp"
    echo ":Nread scp://[user@]machine/file     uses scp"
	break
   elseif match(choice,"^\"") != -1
"    Decho "DBG: reconstructing choice"
    if match(choice,"\"$") != -1
     " case "..."
	 let choice=strpart(choice,1,strlen(choice)-2)
    else
    "  case "... ... ..."
	 let choice      = strpart(choice,1,strlen(choice)-1)
     let wholechoice = ""

     while match(choice,"\"$") == -1
      let wholechoice = wholechoice . " " . choice
      let ichoice     = ichoice + 1
      exe "let choice= a:" . ichoice
     endwhile
	 let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
    endif
   endif
  endif
"  Decho "DBG: NetRead2: choice<" . choice . ">"
  let ichoice= ichoice + 1

  " fix up windows urls
  if has("win32")
    let choice= substitute(choice,'\\','/','ge')
  endif

  " Determine method of read (ftp, rcp, etc)
  call s:NetMethod(choice)

  " Perform Read
  if     s:netrw_method  == 1	" read with rcp
"   Decho "DBG: read via rcp (method #1)"
   exe "!rcp " . s:netrw_machine . ":" . s:netrw_fname . " " . tmpfile
   let result = s:NetGetFile(readcmd, tmpfile)
   let b:netrw_lastfile = choice

  elseif s:netrw_method  == 2		" read with ftp + <.netrc>
"   Decho "DBG: read via ftp+.netrc (method #2)\n"
   exe "norm mzoascii\<cr>get ".s:netrw_fname." ".tmpfile."\<esc>"
   exe "'z+1,.!ftp -i " . s:netrw_machine
   let result = s:NetGetFile(readcmd, tmpfile)
   let b:netrw_lastfile = choice

  elseif s:netrw_method == 3		" read with ftp + machine, id, passwd, and fname
"   Decho "DBG: read via ftp+mipf (method #3)"

   " Seems to depend on the machine.  Dunno how to choose...
   if exists("g:netrw_ftp")
     if g:netrw_ftp == 1
       exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>".g:netrw_uid."\<cr>".g:netrw_passwd."\<cr>ascii\<cr>get ".s:netrw_fname." ".tmpfile."\<esc>"
	 else
       exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>user ".g:netrw_uid." ".g:netrw_passwd."\<cr>ascii\<cr>get ".s:netrw_fname." ".tmpfile."\<esc>"
	 endif
   else
     exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>user ".g:netrw_uid." ".g:netrw_passwd."\<cr>ascii\<cr>get ".s:netrw_fname." ".tmpfile."\<esc>"
   endif

   if has("win32")
    exe "norm o\<esc>my'zj"
    exe ".,'y-1!ftp -i -n"
	" the ftp on Win95 puts four lines of trash at the end
	" which the following blots out.  Does Win-NT/98/2000 do the same???
	exe "'y-3,'yd"
   else
    " ordinary ftp
	" -i       : turns off interactive prompting from ftp
    " -n  unix : DON'T use <.netrc>, even though it exists
	" -n  win32: quit being obnoxious about password
    exe "'z+1,.!ftp -i -n"
    norm 'z
   endif
   let result = s:NetGetFile(readcmd, tmpfile)

   " save choice/id/password for future use
   let b:netrw_lastfile = choice

  elseif     s:netrw_method  == 4	" read with scp
"   Decho "DBG: read via scp (method #4)"
   exe "!scp " . s:netrw_machine . ":" . s:netrw_fname . " " . tmpfile
   let result = s:NetGetFile(readcmd, tmpfile)
   let b:netrw_lastfile = choice

  elseif     s:netrw_method  == 5	" read with http (wget)
"   Decho "DBG: read via http (method #5)"
   if match(s:netrw_fname,"#") == -1
     exe "!wget http://" . s:netrw_machine . "/" . s:netrw_fname . " -O " . tmpfile
     let result = s:NetGetFile(readcmd, tmpfile)
   else
	 let netrw_html= substitute(s:netrw_fname,"#.*$","","")
	 let netrw_tag = substitute(s:netrw_fname,"^.*#","","")
     exe "!wget http://" . s:netrw_machine . "/" . netrw_html . " -O " . tmpfile
     let result = s:NetGetFile(readcmd, tmpfile)
	 exe 'norm 1G/<\s*a\s*name=\s*"'.netrw_tag.'"'
   endif
   set ft=html
   redraw!
   let b:netrw_lastfile = choice

  else " Complain
   echo "***warning*** unable to comply with your request<" . choice . ">"
  endif
 endwhile

 " cleanup
" Decho "DBG NetRead: cleanup"
 if exists("s:netrw_method")
   unlet s:netrw_method
   unlet s:netrw_machine
   unlet s:netrw_fname
 endif
 let &cin  = cinkeep
 let &ai   = aikeep
 let &cino = cinokeep
 let &com  = comkeep
 let &cpo  = cpokeep

" Decho "DBG: return NetRead }"
endfunction
" end of NetRead

" ------------------------------------------------------------------------

" NetGetFile: Function to read file "fname" with command "readcmd".
" Takes care of deleting the last line when the buffer was emtpy.
" Deletes the file "fname".
function! s:NetGetFile(readcmd, fname)
  let dodel = 0
  if line("$") == 1 && getline(1) == ""
    let dodel = 1
  endif
  exe a:readcmd . v:cmdarg . " " . a:fname
  if a:readcmd[0] == '0' && dodel && getline("$") == ""
    $d
    1
  endif
  return delete(a:fname)
endfun

" ------------------------------------------------------------------------

" NetWrite: responsible for writing a file over the net
function! s:NetWrite(...) range
" Decho "DBG: NetWrite(a:0=".a:0.") {"

 " option handling
 let cinkeep=&cin
 let aikeep=&ai
 let cinokeep=&cino
 let comkeep=&com
 let cpokeep=&cpo
 set nocin noai
 set cino=
 set com=
 set cpo-=aA

 " Get Temporary Filename
 let tmpfile    = tempname()
 let tmpFTPfile = tempname()

 if a:0 == 0
  let ichoice = 0
 else
  let ichoice = 1
 endif

 " write (selected portion of) file to temporary
 exe a:firstline . "," . a:lastline . "w!" . v:cmdarg . " " . tmpfile

 while ichoice <= a:0

  " attempt to repeat with previous host-file-etc
  if exists("b:netrw_lastfile") && a:0 == 0
"	 Decho "DBG: using b:netrw_lastfile<" . b:netrw_lastfile . ">"
   let choice = b:netrw_lastfile
   let ichoice= ichoice + 1
  else
   exe "let choice= a:" . ichoice

   " Reconstruct Choice if choice starts with '"'
   if match(choice,"?") == 0
    echo "NetWrite Usage:"
    echo ":Nwrite machine:file                  uses rcp"
    echo ":Nwrite \"machine file\"                uses ftp with <.netrc>"
    echo ":Nwrite \"machine id password file\"    uses ftp"
    echo ":Nwrite ftp://machine[#port]/file          uses ftp  (autodetects <.netrc>)"
    echo ":Nwrite rcp://machine/file          uses rcp"
    echo ":Nwrite scp://[user@]machine/file   uses scp"
	break
   elseif match(choice,"^\"") != -1
    if match(choice,"\"$") != -1
     " case "..."
	 let choice=strpart(choice,1,strlen(choice)-2)
    else
    "  case "... ... ..."
	 let choice      = strpart(choice,1,strlen(choice)-1)
     let wholechoice = ""

     while match(choice,"\"$") == -1
      let wholechoice= wholechoice . " " . choice
      let ichoice= ichoice + 1
      exe "let choice= a:" . ichoice
     endwhile
	let choice= strpart(wholechoice,1,strlen(wholechoice)-1) . " " . strpart(choice,0,strlen(choice)-1)
    endif
   endif
  endif
"  Decho "DBG: choice<" . choice . ">"
  let ichoice= ichoice + 1

  " fix up windows urls
  if has("win32")
    let choice= substitute(choice,'\\','/','ge')
  endif

  " Determine method of read (ftp, rcp, etc)
  call s:NetMethod(choice)

  " Perform Write
  if     s:netrw_method == 1	" write with rcp
   exe "!rcp " . tmpfile . " " . s:netrw_machine . ":" . s:netrw_fname
   let b:netrw_lastfile = choice

  elseif s:netrw_method == 2	" write with ftp + <.netrc>
   exe "norm mzoascii\<cr>put ".tmpfile." ".s:netrw_fname."\<esc>"
   exe "'z+1,.!ftp -i " . s:netrw_machine
   norm 'z
   let b:netrw_lastfile = choice

  elseif s:netrw_method == 3	" write with ftp + machine, id, passwd, and fname
   if exists("g:netrw_ftp")
     if g:netrw_ftp == 1
       exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>".g:netrw_uid."\<cr>".g:netrw_passwd."\<cr>ascii\<cr>put ".tmpfile." ".s:netrw_fname."\<esc>"
	 else
       exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>"."user ".g:netrw_uid." ".g:netrw_passwd."\<cr>ascii\<cr>put ".tmpfile." ".s:netrw_fname."\<esc>"
     endif
   else
     exe "norm mzoopen ".s:netrw_machine." ".s:netrw_port."\<cr>"."user ".g:netrw_uid." ".g:netrw_passwd."\<cr>ascii\<cr>put ".tmpfile." ".s:netrw_fname."\<esc>"
   endif

   if has("win32")
    exe "'z+1,.!ftp -i -n"
	norm u
   else
    " DON'T use <.netrc>, even though it exists
    exe "'z+1,.!ftp -i -n"
   endif
   " save choice/id/password for future use
   let b:netrw_lastfile = choice
   let g:netrw_uid     = g:netrw_uid

  elseif     s:netrw_method == 4	" write with scp
   exe "!scp " . tmpfile . " " . s:netrw_machine . ":" . s:netrw_fname
   let b:netrw_lastfile = choice

  else " Complain
   echo "***warning*** unable to comply with your request<" . choice . ">"
  endif
 endwhile

 " cleanup
" Decho "DBG: NetWrite: cleanup"
 let result=delete(tmpfile)
 if exists("s:netrw_method")
   unlet s:netrw_method
   unlet s:netrw_machine
   unlet s:netrw_fname
 endif
 let &cin  = cinkeep
 let &ai   = aikeep
 let &cino = cinokeep
 let &com  = comkeep
 let &cpo  = cpokeep

 if a:firstline == 1 && a:lastline == line("$")
   set nomod
 endif

 " restore position
 if b:netrw_col == 0
   exe "norm ".b:netrw_line."G0"
 else
   exe "norm ".b:netrw_line."G0".b:netrw_col."l"
 endif

" Decho "DBG: return NetWrite }"
endfunction
" end of NetWrite

" ------------------------------------------------------------------------

" NetMethod:  determine method of transfer
"  method == 1: rcp
"            2: ftp + <.netrc>
"            3: ftp + machine, id, password, and [path]filename
"            4: scp
"            5: http (wget)
function! s:NetMethod(choice)  " globals: method machine id passwd fname
" Decho "DBG: NetMethod(a:choice<".a:choice.">) {"

 " initialization
 let s:netrw_method  = 0
 let s:netrw_machine = ""
 let s:netrw_fname   = ""
 let s:netrw_port    = ""

 " Patterns:
 " mipf   : a:machine a:id password filename  Use ftp
 " mf     : a:machine filename                Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
 " ftpurm : ftp://host[#port]/filename        Use ftp + <.netrc> or g:netrw_uid g:netrw_passwd
 " rcpurm : rcp://host/filename               Use rcp
 " rcphf  : host:filename                     Use rcp
 " scpurm : scp://[user@]host/filename        Use scp
 " httpurm: http://[user@]host/filename       Use wget
 let mipf   = '\(\S\+\)\s\+\(\S\+\)\s\+\(\S\+\)\s\+\(\S\+\)'
 let mf     = '\(\S\+\)\s\+\(\S\+\)'
 let ftpurm = 'ftp://\([^/#]\{-}\)\(#\d\+\)\=/\(.*\)$'
 let rcpurm = 'rcp://\([^/]\{-}\)/\(.*\)$'
 let rcphf  = '\(\I\i*\):\(\S\+\)'
 let scpurm = 'scp://\([^/]\{-}\)/\(.*\)$'
 let httpurm= 'http://\([^/]\{-}\)/\(.*\)$'

 " Determine Method
 " rcp://hostname/...path-to-file
 if match(a:choice,rcpurm) == 0
"  Decho "DBG: NetMethod: rcp://..."
  let s:netrw_method = 1
  let s:netrw_machine= substitute(a:choice,rcpurm,'\1',"")
  let s:netrw_fname  = substitute(a:choice,rcpurm,'\2',"")

 " scp://user@hostname/...path-to-file
 elseif match(a:choice,scpurm) == 0
"  Decho "DBG: NetMethod: scp://..."
  let s:netrw_method = 4
  let s:netrw_machine= substitute(a:choice,scpurm,'\1',"")
  let s:netrw_fname  = substitute(a:choice,scpurm,'\2',"")

 " http://hostname/...path-to-file
 elseif match(a:choice,httpurm) == 0
"  Decho "DBG: NetMethod: http://..."
  let s:netrw_method = 5
  let s:netrw_machine= substitute(a:choice,httpurm,'\1',"")
  let s:netrw_fname  = substitute(a:choice,httpurm,'\2',"")

 " ftp://hostname/...path-to-file
 elseif match(a:choice,ftpurm) == 0
"  Decho "DBG: NetMethod: ftp://..."
  let s:netrw_machine= substitute(a:choice,ftpurm,'\1',"")
  let s:netrw_port   = substitute(a:choice,ftpurm,'\2',"")
  let s:netrw_fname  = substitute(a:choice,ftpurm,'\3',"")
  if s:netrw_port != ""
    let s:netrw_port = substitute(s:netrw_port,"#","","")
  endif
  if exists("g:netrw_uid") && exists("g:netrw_passwd")
   let s:netrw_method = 3
  else
   if filereadable(expand("$HOME/.netrc"))
	 let s:netrw_method= 2
   else
     if !exists("g:netrw_uid") || g:netrw_uid == ""
       call NetUserPass()
	 elseif !exists("g:netrw_passwd") || g:netrw_passwd == ""
       call NetUserPass(g:netrw_uid)
	 " else just use current g:netrw_uid and g:netrw_passwd
	 endif
	 let s:netrw_method= 3
	endif
  endif

 " Issue an rcp: hostname:filename"
 elseif match(a:choice,rcphf) == 0
"  Decho "DBG: NetMethod: (rcp) host:file"
  let s:netrw_method = 1
  let s:netrw_machine= substitute(a:choice,rcphf,'\1',"")
  let s:netrw_fname  = substitute(a:choice,rcphf,'\2',"")
  if has("win32")
   " don't let PCs try <.netrc>
   let s:netrw_method = 3
  endif

 " Issue an ftp : "machine id password [path/]filename"
 elseif match(a:choice,mipf) == 0
"  Decho "DBG: NetMethod: (ftp) host id pass file"
  let s:netrw_method  = 3
  let s:netrw_machine = substitute(a:choice,mipf,'\1',"")
  let g:netrw_uid     = substitute(a:choice,mipf,'\2',"")
  let g:netrw_passwd  = substitute(a:choice,mipf,'\3',"")
  let s:netrw_fname   = substitute(a:choice,mipf,'\4',"")

 " Issue an ftp: "hostname [path/]filename"
 elseif match(a:choice,mf) == 0
"  Decho "DBG: NetMethod: (ftp) host file"
  if exists("g:netrw_uid") && exists("g:netrw_passwd")
   let s:netrw_method  = 3;
   let s:netrw_machine = substitute(a:choice,mf,'\1',"")
   let s:netrw_fname   = substitute(a:choice,mf,'\2',"")

  elseif filereadable(expand("$HOME/.netrc"))
   let s:netrw_method  = 2
   let s:netrw_machine = substitute(a:choice,mf,'\1',"")
   let s:netrw_fname   = substitute(a:choice,mf,'\2',"")
  endif

 else
   echoerr "***error*** cannot determine method"
   let s:netrw_method  = -1
 endif

" call Decho("DBG: NetMethod: a:choice       <".a:choice.">")
" call Decho("DBG: NetMethod: s:netrw_method <".s:netrw_method.">")
" call Decho("DBG: NetMethod: s:netrw_machine<".s:netrw_machine.">")
" call Decho("DBG: NetMethod: s:netrw_port   <".s:netrw_port.">")
" if exists("g:netrw_uid")		" DBG
"  call Decho("DBG: NetMethod: g:netrw_uid    <".g:netrw_uid.">")
" endif							" DBG
" if exists("g:netrw_passwd")	" DBG
"  call Decho("DBG: NetMethod: g:netrw_passwd <".g:netrw_passwd.">")
" endif							" DBG
" call Decho("DBG: NetMethod: s:netrw_fname  <".s:netrw_fname.">")
" call Decho("DBG: NetMethod return }")
endfunction
" end of NetMethod

" ------------------------------------------------------------------------

" NetUserPass: set username and password for subsequent ftp transfer
"   Usage:  :call NetUserPass()                 -- will prompt for userid and password
"           :call NetUserPass("uid")            -- will prompt for password
"           :call NetUserPass("uid","password") -- sets global userid and password
function! NetUserPass(...)

 " get/set userid
 if a:0 == 0
"  Decho "DBG: NetUserPass(a:0<".a:0.">) {"
  if !exists("g:netrw_uid") || g:netrw_uid == ""
   " via prompt
   let g:netrw_uid= input('Enter username: ')
  endif
 else
  " from command line
"  Decho "DBG: NetUserPass(a:1<".a:1.">) {"
  let g:netrw_uid= a:1
 endif

 " get password
 if a:0 <= 1
   " via prompt
"  Decho "DBG: a:0=".a:0." case <=1:"
  let g:netrw_passwd= inputsecret("Enter Password: ")
 else
  " from command line
"  Decho "DBG: a:0=".a:0." case >1: a:2<".a:2.">"
  let g:netrw_passwd=a:2
 endif
" Decho "DBG: return NetUserPass }"
endfunction
" end NetUserPass

" ------------------------------------------------------------------------

" Restore
let &cpo= s:save_cpo
unlet s:save_cpo
