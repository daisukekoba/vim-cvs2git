" Vim settings file
" Language:     OCaml
" Maintainers:  Mike Leary    <leary@nwlink.com>
"               Markus Mottl  <markus@oefai.at>
" URL:          http://www.ai.univie.ac.at/~markus/vim/ftplugin/ocaml.vim
" Last Change:  2001 Sep 02 - initial release  (MM)

" Only do these settings when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't do other file type settings for this buffer
let b:did_ftplugin = 1

" Error formats
setlocal efm=
  \%EFile\ \"%f\"\\,\ line\ %l\\,\ characters\ %c-%*\\d:,
  \%EFile\ \"%f\"\\,\ line\ %l\\,\ character\ %c:%m,
  \%+EReference\ to\ unbound\ regexp\ name\ %m,
  \%Eocamlyacc:\ e\ -\ line\ %l\ of\ \"%f\"\\,\ %m,
  \%Wocamlyacc:\ w\ -\ %m,
  \%-Zmake%.%#,
  \%C%m

" Add mappings, unless the user didn't want this.
if !exists("no_plugin_maps") && !exists("no_mail_maps")
  " Uncommenting
  if !hasmapto('<Plug>Comment')
    nmap <buffer> <LocalLeader>c <Plug>LUncomOn
    vmap <buffer> <LocalLeader>c <Plug>BUncomOn
    nmap <buffer> <LocalLeader>C <Plug>LUncomOff
    vmap <buffer> <LocalLeader>C <Plug>BUncomOff
  endif

  nnoremap <buffer> <Plug>LUncomOn mz0i(*<ESC>$A*)<ESC>`z
  nnoremap <buffer> <Plug>LUncomOff <ESC>:s/^(\*\(.*\)\*)/\1/<CR>
  vnoremap <buffer> <Plug>BUncomOn <ESC>:'<,'><CR>`<O<ESC>0i(*<ESC>`>o<ESC>0i*)<ESC>`<
  vnoremap <buffer> <Plug>BUncomOff <ESC>:'<,'><CR>`<dd`>dd`<
endif
