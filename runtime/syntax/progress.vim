" Vim syntax file
" Language:				Progress 4GL
" Filename extensions:	*.p (collides with Pascal),
"						*.i (collides with assembler)
"						*.w (collides with cweb)
" Maintainer:			Philip Uren			<philu@computer.org>
" Contributors:			Chris Ruprecht		<chrup@mac.com>
"						Philip Uren			<philu@computer.org>
"						Mikhail Kuperblum	<mikhail@whasup.com>
" URL: 					http://www.zeta.org.au/~philu/vim/progress.vim
" Last Change:			Fri Mar 30 09:04:39 EST 2001

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

set isk=@,48-57,_,-,!,#,$,%
" Remove any old syntax stuff hanging around

syn case ignore

" Progress Blocks of code and mismatched "end." errors.
syn match   ProgressEndError		"end\."
syn region ProgressDoBlock transparent matchgroup=ProgressDo start="do" matchgroup=ProgressDo end="end\." contains=ALLBUT,ProgressProcedure,ProgressFunction
syn region ProgressForBlock transparent matchgroup=ProgressFor start="^[ 	]*for\>" matchgroup=ProgressFor end="end\." contains=ALLBUT,ProgressProcedure,ProgressFunction
syn region ProgressRepeatBlock transparent matchgroup=ProgressRepeat start="^[ 	]*repeat\>" matchgroup=ProgressRepeat end="end\." contains=ALLBUT,ProgressProcedure,ProgressFunction

" These are Progress reserved words,
" and they could go in ProgressReserved,
" but I found it more helpful to highlight them in a different color.
syn keyword ProgressConditional	if else then when otherwise case
syn keyword ProgressFor				each where

" Make those TODO and debugging notes stand out!
syn keyword	ProgressTodo			contained	TODO BUG FIX
syn keyword ProgressDebug			contained	DEBUG
syn keyword ProgressDebug						debugger
syn match	ProgressNeedsWork		contained	"NEED[S]*[ 	][ 	]*WORK"

" If you like to highlight the whole line of
" the start and end of procedures
" to make the whole block of code stand out:
syn match ProgressProcedure		"^[ 	]*procedure.*"
syn match ProgressProcedure		"^[ 	]*end[ 	][ 	]*procedure.*"

syn match ProgressFunction		"^[ 	]*function.*"
syn match ProgressFunction		"^[ 	]*end[ 	][ 	]*function.*"
" ... otherwise use this:
" syn keyword ProgressFunction	procedure function

syn keyword ProgressReserved	accum[ulate] active-window add alias all alter ambig[uous] analyz[e] and any apply as asc[ending] assign at attr[-space]
syn keyword ProgressReserved	authorization auto-ret[urn] avail[able] back[ground] before-h[ide] begins bell between blank break btos by call can-do can-find
syn keyword ProgressReserved	center[ed] check chr clear clipboard col colon color col[umn] column-lab[el] col[umns] compiler connected control count-of
syn keyword ProgressReserved	cpstream create ctos current current-changed current-lang[uage] current-window current_date curs[or] database dataservers
syn keyword ProgressReserved	dbcodepage dbcollation dbname dbrest[rictions] dbtaskid dbtype dbvers[ion] dde deblank debug-list debugger decimals declare
syn keyword ProgressReserved	def default default-noxl[ate] default-window def[ine] delete delimiter desc[ending] dict[ionary] disable discon[nect] disp
syn keyword ProgressReserved	disp[lay] distinct dos down drop editing enable encode entry error-stat[us] escape etime except exclusive
syn keyword ProgressReserved	exclusive[-lock] exclusive-web-us[er] exists export false fetch field field[s] file-info[rmation] fill find find-case-sensitive
syn keyword ProgressReserved	find-global find-next-occurrence find-prev-occurrence find-select find-wrap-around first first-of focus font form form[at]
syn keyword ProgressReserved	fram[e] frame-col frame-db frame-down frame-field frame-file frame-inde[x] frame-line frame-name frame-row frame-val[ue]
syn keyword ProgressReserved	from from-c[hars] from-p[ixels] gateway[s] get-byte get-codepage[s] get-coll[ations] get-key-val[ue] getbyte global go-on
syn keyword ProgressReserved	go-pend[ing] grant graphic-e[dge] group having header help hide import in index indicator input input-o[utput] insert
syn keyword ProgressReserved	into is is-attr[-space] join kblabel key-code key-func[tion] key-label keycode keyfunc[tion] keylabel keys keyword label
syn keyword ProgressReserved	last last-even[t] last-key last-of lastkey ldbname leave library like line-count[er] listi[ng] locked lookup machine-class
syn keyword ProgressReserved	map member message message-lines mouse mpe new next next-prompt no no-attr[-space] no-error no-f[ill] no-help no-hide no-label[s]
syn keyword ProgressReserved	no-lock no-map no-mes[sage] no-pause no-prefe[tch] no-undo no-val[idate] no-wait not null num-ali[ases] num-dbs num-entries
syn keyword ProgressReserved	of off old on open opsys option or os-append os-command os-copy os-create-dir os-delete os-dir os-drive[s] os-error os-rename
syn keyword ProgressReserved	os2 os400 output overlay page page-bot[tom] page-num[ber] page-top param[eter] pause pdbname persist[ent] pixels
syn keyword ProgressReserved	preproc[ess] privileges proc-ha[ndle] proc-st[atus] process program-name Progress prompt prompt[-for] promsgs propath provers[ion]
syn keyword ProgressReserved	put put-byte put-key-val[ue] putbyte query query-tuning quit r-index rcode-informatio[n] readkey recid record-len[gth] rect[angle]
syn keyword ProgressReserved	release reposition retain retry return return-val[ue] revert revoke run save schema screen screen-io screen-lines
syn keyword ProgressReserved	scroll sdbname search seek select self session set setuser[id] share[-lock] shared show-stat[s] skip some space status stream
syn keyword ProgressReserved	stream-io string-xref system-dialog table term term[inal] text text-cursor text-seg[-growth] this-procedure time title
syn keyword ProgressReserved	to today top-only trans trans[action] trigger triggers trim true underl[ine] undo unform[atted] union unique unix up update
syn keyword ProgressReserved	use-index use-revvideo use-underline user user[id] using v6frame value values view view-as vms wait-for web-con[text]
syn keyword ProgressReserved	window window-maxim[ized] window-minim[ized] window-normal with work-tab[le] workfile write xcode xref yes _cbit
syn keyword ProgressReserved	_control _list _memory _msg _pcontrol _serial[-num] _trace 
syn keyword ProgressTodo        contained	TODO

" Strings. Handles embedded quotes.
" Note that, for some reason, Progress doesn't use the backslash, "\"
" as the escape character; it uses tilde, "~".
syn region ProgressString	matchgroup=ProgressQuote	start=+"+ end=+"+	skip=+\~"+
syn region ProgressString	matchgroup=ProgressQuote	start=+'+ end=+'+	skip=+\~'+

syn match  ProgressIdentifier		"\<[a-zA-Z_][a-zA-Z0-9_]*\>()"

" syn match  ProgressDelimiter		"()"

" syn match  ProgressMatrixDelimiter	"[][]"

" If you prefer you can highlight the range
"syn match  ProgressMatrixDelimiter	"[\d\+\.\.\d\+]"

syn match  ProgressNumber		"\<\d\+\(u\=l\=\|lu\|f\)\>"
syn match  ProgressByte		"\$[0-9a-fA-F]\+"

" If you don't like tabs
"syn match ProgressShowTab "\t"
"syn match ProgressShowTabc "\t"

syn region ProgressComment		start="/\*"  end="\*/" contains=ProgressComment,ProgressTodo,ProgressDebug,ProgressNeedsWork
syn match ProgressInclude		"^[ 	]*[{].*\.i[}]"

syn match ProgressSubstitute	"^[ 	]*[{].*[^i][}]"
syn match ProgressPreProc		"^[ 	]*&.*"

syn match ProgressOperator		"[!;|)(:.><+*=-]"

syn keyword ProgressOperator	<= <> >= abs[olute] accelerator across add-first add-last advise alert-box allow-replication ansi-only anywhere append appl-alert[-boxes] application as-cursor ask-overwrite
syn keyword ProgressOperator	attach[ment] auto-end-key auto-endkey auto-go auto-ind[ent] auto-resize auto-z[ap] available-formats ave[rage] avg backward[s] base-key batch[-mode] bgc[olor] binary
syn keyword ProgressOperator	bind-where block-iteration-display border-bottom border-bottom-ch[ars] border-bottom-pi[xels] border-left border-left-char[s] border-left-pixe[ls] border-right border-right-cha[rs]
syn keyword ProgressOperator	border-right-pix[els] border-t[op] border-t[op-chars] border-top-pixel[s] both bottom box box-select[able] browse browse-header buffer buffer-chars buffer-lines
syn keyword ProgressOperator	button button[s] byte cache cache-size can-query can-set cancel-break cancel-button caps careful-paint c[ase-sensitive] cdecl char[acter] character_length charset
syn keyword ProgressOperator	checked choose clear-select[ion] close code codepage codepage-convert col-of colon-align[ed] color-table column-bgc[olor] column-dcolor column-fgc[olor] column-font
syn keyword ProgressOperator	column-label-bgc[olor] column-label-dcolor column-label-fgc[olor] column-label-font column-of column-pfc[olor] column-sc[rolling] combo-box command compile complete
syn keyword ProgressOperator	connect constrained contents context context-pop[up] control-containe[r] c[ontrol-form] convert-to-offse[t] convert count cpcase cpcoll cpint[ernal] cplog
syn keyword ProgressOperator	cpprint cprcodein cprcodeout cpterm crc-val[ue] c[reate-control] create-result-list-entry create-test-file current-column current-environm[ent] current-iteration
syn keyword ProgressOperator	current-result-row current-row-modified current-value cursor-char cursor-line cursor-offset data-entry-retur[n] data-t[ype] date date-f[ormat] day db-references
syn keyword ProgressOperator	dcolor dde-error dde-i[d] dde-item dde-name dde-topic debu[g] dec[imal] default-b[utton] default-extensio[n] defer-lob-fetch defined delete-char delete-current-row
syn keyword ProgressOperator	delete-line delete-selected-row delete-selected-rows deselect-focused-row deselect-rows deselect-selected-row d[esign-mode] dialog-box dialog-help dir disabled display-message
syn keyword ProgressOperator	display-t[ype] double drag-enabled drop-down drop-down-list dump dynamic echo edge edge[-chars] edge-p[ixels] editor empty end-key endkey entered eq error error-col[umn]
syn keyword ProgressOperator	error-row event-t[ype] event[s] exclusive-id execute exp expand extended extent external extract fetch-selected-row fgc[olor] file file-name file-off[set] file-type
syn keyword ProgressOperator	filename fill-in filled filters first-child first-column first-proc[edure] first-tab-i[tem] fixed-only float focused-row font-based-layout font-table force-file
syn keyword ProgressOperator	fore[ground] form-input forward[s] frame-spa[cing] frame-x frame-y frequency from-cur[rent] full-height full-height-char[s] full-height-pixe[ls] full-pathn[ame]
syn keyword ProgressOperator	full-width full-width[-chars] full-width-pixel[s] ge get get-blue[-value] g[et-char-property] get-double get-dynamic get-file get-float get-green[-value]
syn keyword ProgressOperator	get-iteration get-license get-long get-message get-number get-pointer-value get-red[-value] get-repositioned-row get-selected-wid[get] get-short get-signature get-size
syn keyword ProgressOperator	get-string get-tab-item get-text-height get-text-height-char[s] get-text-height-pixe[ls] get-text-width get-text-width-c[hars] get-text-width-pixel[s] get-unsigned-short
syn keyword ProgressOperator	grayed grid-factor-horizont[al] grid-factor-vert[ical] grid-set grid-snap grid-unit-height grid-unit-height-cha[rs] grid-unit-height-pix[els] grid-unit-width grid-unit-width-char[s]
syn keyword ProgressOperator	grid-unit-width-pixe[ls] grid-visible gt handle height height[-chars] height-p[ixels] help-con[text] helpfile-n[ame] hidden hint hori[zontal] hwnd image image-down
syn keyword ProgressOperator	image-insensitive image-size image-size-c[hars] image-size-pixel[s] image-up immediate-display index-hint indexed-reposition info[rmation] init init[ial] initial-dir
syn keyword ProgressOperator	initial-filter initiate inner inner-chars inner-lines insert-b[acktab] insert-file insert-row insert-string insert-t[ab] int[eger] internal-entries is-lead-byte
syn keyword ProgressOperator	is-row-selected is-selected item items-per-row join-by-sqldb keep-frame-z-ord[er] keep-messages keep-tab-order key keyword-all label-bgc[olor] label-dc[olor] label-fgc[olor]
syn keyword ProgressOperator	label-font label-pfc[olor] labels language[s] large large-to-small last-child last-tab-i[tem] last-proce[dure] lc le leading left left-align[ed] left-trim length
syn keyword ProgressOperator	line list-events list-items list-query-attrs list-set-attrs list-widgets load l[oad-control] load-icon load-image load-image-down load-image-insensitive load-image-up
syn keyword ProgressOperator	load-mouse-point[er] load-small-icon log logical lookahead lower lt manual-highlight margin-extra margin-height margin-height-ch[ars] margin-height-pi[xels] margin-width
syn keyword ProgressOperator	margin-width-cha[rs] margin-width-pix[els] matches max max-chars max-data-guess max-height max-height[-chars] max-height-pixel[s] max-rows max-size max-val[ue] max-width
syn keyword ProgressOperator	max-width[-chars] max-width-p[ixels] maximize max[imum] memory menu menu-bar menu-item menu-k[ey] menu-m[ouse] menubar message-area message-area-font message-line
syn keyword ProgressOperator	min min-height min-height[-chars] min-height-pixel[s] min-size min-val[ue] min-width min-width[-chars] min-width-p[ixels] min[imum] mod modified mod[ulo] month mouse-p[ointer]
syn keyword ProgressOperator	movable move-after-tab-i[tem] move-before-tab-[item] move-col[umn] move-to-b[ottom] move-to-eof move-to-t[op] multiple multiple-key multitasking-interval must-exist
syn keyword ProgressOperator	name native ne new-row next-col[umn] next-sibling next-tab-ite[m] next-value no-apply no-assign no-bind-where no-box no-column-scroll[ing] no-convert no-current-value
syn keyword ProgressOperator	no-debug no-drag no-echo no-index-hint no-join-by-sqldb no-lookahead no-row-markers no-scrolling no-separate-connection no-separators no-und[erline] no-word-wrap
syn keyword ProgressOperator	none num-but[tons] num-col[umns] num-copies num-formats num-items num-iterations num-lines num-locked-colum[ns] num-messages num-results num-selected num-selected-rows
syn keyword ProgressOperator	num-selected-widgets num-tabs num-to-retain numeric numeric-f[ormat] octet_length ok ok-cancel on-frame[-border] ordered-join ordinal orientation os-getenv outer
syn keyword ProgressOperator	outer-join override owner page-size page-wid[th] paged parent partial-key pascal pathname pfc[olor] pinnable pixels-per-colum[n] pixels-per-row popup-m[enu] popup-o[nly]
syn keyword ProgressOperator	position precision presel[ect] prev prev-col[umn] prev-sibling prev-tab-i[tem] primary printer-control-handle printer-setup private-d[ata] proce[dure] Progress-s[ource]
syn keyword ProgressOperator	put-double put-float put-long put-short put-string put-unsigned-short query-off-end question radio-buttons radio-set random raw raw-transfer read-file read-only
syn keyword ProgressOperator	real recursive refresh refreshable replace replace-selection-text replication-create replication-delete replication-write request resiza[ble] resize retry-cancel
syn keyword ProgressOperator	return-ins[erted] return-to-start-di[r] reverse-from right right-align[ed] right-trim round row row-ma[rkers] row-of rowid rule rule-row rule-y save-as save-file
syn keyword ProgressOperator	screen-val[ue] scroll-bars scroll-delta scroll-horiz-value scroll-offset scroll-to-current-row scroll-to-i[tem] scroll-to-selected-row scroll-vert-value scrollable
syn keyword ProgressOperator	scrollbar-horizo[ntal] scrollbar-vertic[al] scrolled-row-positio[n] scrolling se-check-pools se-enable-of[f] se-enable-on se-num-pools se-use-messa[ge] section select-focused-row
syn keyword ProgressOperator	select-next-row select-prev-row select-repositioned-row select-row selectable selected selected-items selection-end selection-list selection-start selection-text
syn keyword ProgressOperator	send sensitive separate-connection separators set-blue[-value] set-break set-cell-focus set-contents set-dynamic set-green[-value] set-leakpoint set-pointer-valu[e]
syn keyword ProgressOperator	s[et-property] set-red[-value] set-repositioned-row set-selection set-size set-wait[-state] side-lab side-lab[e] side-lab[el] side-label-handl[e] side-lab[els] silent
syn keyword ProgressOperator	simple single size size-c[hars] size-p[ixels] slider smallint sort source sql sqrt start status-area status-area-font status-bar stdcall stenciled stop stoppe[d]
syn keyword ProgressOperator	stored-proc[edure] string sub-ave[rage] sub-count sub-max[imum] sub-me[nu] sub-menu-help sub-min[imum] sub-total subst[itute] substr[ing] subtype sum suppress-warning[s]
syn keyword ProgressOperator	system-alert-box[es] system-help tab-position tabbable target temp-dir[ectory] temp-table terminate text-selected three-d through thru tic-marks time-source title-bgc[olor]
syn keyword ProgressOperator	title-dc[olor] title-fgc[olor] title-fo[nt] to-rowid toggle-box tool-bar top topic total trailing trunc[ate] type unbuff[ered] unique-id unload upper use use-dic[t-exps]
syn keyword ProgressOperator	use-filename use-text v6display valid-event valid-handle validate validate-condition validate-message var[iable] vert[ical] virtual-height virtual-height-c[hars]
syn keyword ProgressOperator	virtual-height-pixel[s] virtual-width virtual-width-ch[ars] virtual-width-pi[xels] visible wait warning weekday widget widget-e[nter] widget-h[andle] widget-l[eave]
syn keyword ProgressOperator	widget-pool width width[-chars] width-p[ixels] window-name window-sta[te] window-sys[tem] word-wrap x-of y-of year yes-no yes-no-cancel _dcm
syn keyword ProgressOperator	source-procedure subscribe super target-procedure unsubscribe
" Progress Version 9.0 added these
syn keyword ProgressOperator	profiler publish source-procedure subscribe super target-procedure unsubscribe profiler

syn keyword ProgressType	    char[acter] int[eger] format
syn keyword ProgressType	    var[iable] log[ical] da[te]

syn sync lines=2500

" The default highlighting.
hi def link ProgressAcces			Statement
hi def link ProgressByte			Number
hi def link ProgressComment			Comment
" hi link ProgressComment			StatusLine
hi def link ProgressConditional		Conditional
hi def link ProgressDebug			Debug
hi def link ProgressDo				Repeat
hi def link ProgressEndError		Error
hi def link ProgressFor				Repeat
hi def link ProgressFunction		Function
" hi link ProgressFunction			Procedure
hi def link ProgressInclude			Include
hi def link ProgressLabel			Label
hi def link ProgressMatrixDelimiter	Identifier
hi def link ProgressModifier		Type
hi def link ProgressNeedsWork		Todo
hi def link ProgressNumber			Number
hi def link ProgressOperator		Operator
" hi link ProgressOperator			Function
hi def link ProgressPreProc			PreProc
hi def link ProgressProcedure		Procedure
hi def link ProgressQuote			Delimiter
hi def link ProgressRepeat			Repeat
hi def link ProgressReserved		Statement
" hi link ProgressReserved			Identifier
" hi link ProgressStatement			Statement
hi def link ProgressString			String
hi def link ProgressStructure		Structure
hi def link ProgressSubstitute		PreProc
hi def link ProgressTodo			Todo
hi def link ProgressType			Statement
hi def link ProgressUnclassified	Statement

"optional highlighting
"hi link ProgressDelimiter			Identifier
"hi link ProgressShowTab			Error
"hi link ProgressShowTabc			Error
"hi link ProgressIdentifier			Identifier

let b:current_syntax = "progress"

" vim: ts=4 sw=2 
