" Menu Translations:	Korean
" Maintainer:		Nam SungHyun <namsh@kldp.org>
" Last Change:		2001 Sep 02

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding euc-kr

" Help menu
menutrans &Help			도움말(&H)
menutrans &Overview<Tab><F1>	개관(&O)<Tab><F1>
menutrans &How-to\ links	하우투\ 색인(&H)
menutrans &GUI			구이(&G)
menutrans &Credits		고마운\ 분들(&C)
menutrans Co&pying		저작권(&p)
menutrans &Find\.\.\.		찾기(&F)\.\.\.
menutrans &Version		버전(&V)
menutrans &About		이\ 프로그램은(&A)

" File menu
menutrans &File				파일(&F)
menutrans &Open\.\.\.<Tab>:e		열기(&O)\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	분할해서\ 열기(&l)\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		새로운(&N)<Tab>:enew
menutrans &Close<Tab>:close		닫기(&C)<Tab>:close
menutrans &Save<Tab>:w			저장(&S)<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	다른\ 이름으로\ 저장(&A)\.\.\.<Tab>:sav
"menutrans Split\ &Diff\ with\.\.\.	Split\ &Diff\ with\.\.\.
"menutrans Split\ Patched\ &By\.\.\.	Split\ Patched\ &By\.\.\.
menutrans &Print			인쇄(&P)
menutrans Sa&ve-Exit<Tab>:wqa		저장하고\ 종료(&v)<Tab>:wqa
menutrans E&xit<Tab>:qa			종료(&x)<Tab>:qa

" Edit menu
menutrans &Edit				편집(&E)
menutrans &Undo<Tab>u			취소(&U)<Tab>u
menutrans &Redo<Tab>^R			재실행(&R)<Tab>^R
menutrans Rep&eat<Tab>\.		반복(&e)<Tab>\.
menutrans Cu&t<Tab>"+x			자르기(&t)<Tab>"+x
menutrans &Copy<Tab>"+y			복사(&C)<Tab>"+y
menutrans &Paste<Tab>"+P		붙이기(&P)<Tab>"+P
menutrans Put\ &Before<Tab>[p		앞에\ 붙이기(&B)<Tab>[p
menutrans Put\ &After<Tab>]p		뒤에\ 붙이기(&A)<Tab>]p
menutrans &Delete<Tab>x			삭제(&D)<Tab>x
menutrans &Select\ all<Tab>ggVG		모두\ 선택(&S)<Tab>ggVG
menutrans &Find\.\.\.			찾기(&F)\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.	찾아\ 바꾸기(&l)\.\.\.
menutrans Settings\ &Window		설정\ 창(&W)

" Edit/Global Settings
menutrans &Global\ Settings		전역\ 설정(&G)
menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls! 패턴\ 하이라이트\ 토글(&H)<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic! 대소문자\ 구분\ 토글(&I)<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm! Showmatch\ 토글(&S)<Tab>:set\ sm!
menutrans &Context\ lines		콘텍스트\ 라인(&C)
menutrans &Virtual\ Edit		가상\ 편집(&V)
menutrans Never				사용않함
menutrans Block\ Selection		블럭\ 선택
menutrans Insert\ mode			삽입\ 모드
menutrans Block\ and\ Insert		블럭과\ 삽입
menutrans Always			항상\ 사용
menutrans Toggle\ Insert\ &Mode<Tab>:set\ im! 삽입\ 모드\ 토글(&M)<Tab>:set\ im!
menutrans Search\ &Path\.\.\.		찾기\ 경로(&P)\.\.\.
menutrans Ta&g\ Files\.\.\.		태그\ 파일(&g)\.\.\.

" GUI options
menutrans Toggle\ &Toolbar		도구막대\ 토글(&T)
menutrans Toggle\ &Bottom\ Scrollbar	바닥\ 스크롤바\ 토글(&B)
menutrans Toggle\ &Left\ Scrollbar	왼쪽\ 스크롤바\ 토글(&L)
menutrans Toggle\ &Right\ Scrollbar	오른쪽\ 스크롤바\ 토글(&R)

" Edit/File Settings
menutrans F&ile\ Settings		파일\ 설정(&i)

" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu! 라인\ 번호붙이기\ 토글(&N)<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list! 목록\ 모드\ 토글(&L)<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap! 라인\ Wrap\ 토글(&W)<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr! 단어에서\ Wrap\ 토글(&r)<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et! 탭\ 확장\ 토글(&e)<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai! 자동인덴트\ 토글(&a)<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin! C-인덴팅\ 토글(&C)<Tab>:set\ cin!

" other options
menutrans &Shiftwidth			쉬프트너비(&S)
menutrans Soft\ &Tabstop		소프트\ 탭스톱(&T)
menutrans Te&xt\ Width\.\.\.		텍스트\ 너비(&x)\.\.\.
menutrans &File\ Format\.\.\.		파일\ 형식(&F)\.\.\.
menutrans C&olor\ Scheme		컬러\ 스킴(&o)
menutrans &Keymap			키맵(&K)

menutrans Select\ Fo&nt\.\.\.		글꼴\ 선택(&F)\.\.\.

" Programming menu
menutrans &Tools			도구(&T)
menutrans &Jump\ to\ this\ tag<Tab>g^]	이\ 태그로\ 점프(&J)<Tab>g^]
menutrans Jump\ &back<Tab>^T		뒤로\ 점프(&b)<Tab>^T
menutrans Build\ &Tags\ File		태그\ 파일\ 생성(&T)

" Tools.Fold Menu
menutrans &Folding			폴딩(&F)
" open close folds
menutrans &Enable/Disable\ folds<Tab>zi	폴드\ 사용\ 토글(&E)<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv	커서\ 라인\ 보기(&V)<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx 커서\ 라인만\ 보기(&w)<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm	더많은\ 폴드\ 닫기(&l)<Tab>zm
menutrans &Close\ all\ folds<Tab>zM	모든\ 폴드\ 닫기(&C)<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr	더많은\ 폴드\ 열기(&p)<Tab>zr
menutrans &Open\ all\ folds<Tab>zR	모든\ 폴드\ 열기(&O)<Tab>zR
" fold method
menutrans Fold\ Met&hod			폴드\ 방법(&h)
menutrans M&anual			수동(&a)
menutrans I&ndent			인덴트(&n)
menutrans E&xpression			표현식(&x)
menutrans S&yntax			신택스(&y)
"menutrans &Diff				&Diff
menutrans Ma&rker			마크(&r)
" create and delete folds
menutrans Create\ &Fold<Tab>zf		폴드\ 생성(&F)<Tab>zf
menutrans &Delete\ Fold<Tab>zd		폴드\ 삭제(&D)<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD	모든\ 폴드\ 삭제(&A)<Tab>zD
" moving around in folds
menutrans Fold\ column\ &width		폴드\ 열\ 너비(&w)

"menutrans &Diff				&Diff
menutrans &Update			갱신(&U)
menutrans &Get\ Block			블럭\ 가져오기(&G)
menutrans &Put\ Block			블럭\ 집어넣기(&P)

menutrans &Make<Tab>:make		Make(&M)<Tab>:make
menutrans &List\ Errors<Tab>:cl		에러\ 목록\ 보기(&L)<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	메시지\ 목록\ 보기(&i)<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		다음\ 에러(&N)<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	이전\ 에러(&P)<Tab>:cp
menutrans &Older\ List<Tab>:cold	오래된\ 목록(&O)<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	새로운\ 목록(&e)<Tab>:cnew
menutrans Error\ &Window<Tab>:cwin	에러\ 창(&W)<Tab>:cwin
menutrans &Convert\ to\ HEX<Tab>:%!mc\ vim:xxd	십육진으로\ 변환<Tab>:%!mc\ vim:xxd
menutrans &Convert\ to\ HEX<Tab>:%!xxd	십육진으로\ 변환(&C)<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!mc\ vim:xxd\ -r	원래대로\ 변환(&r)<Tab>:%!mc\ vim:xxd\ -r
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	원래대로\ 변환(&r)<Tab>:%!xxd\ -r

" Names for buffer menu.
menutrans &Buffers			버퍼(&B)
menutrans &Refresh\ menu		메뉴\ 다시그리기(&R)
menutrans &Delete			삭제(&D)
menutrans &Alternate			교체(&A)
menutrans &Next				다음(&N)
menutrans &Previous			이전(&P)

" Window menu
menutrans &Window			창(&W)
menutrans &New<Tab>^Wn			새\ 창(&N)<Tab>^Wn
menutrans S&plit<Tab>^Ws		분할(&p)<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	#으로\ 분할(&l)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	수직\ 분할(&V)<Tab>^Wv
menutrans Split\ File\ E&xplorer	파일\ 익스플로러\ 분할

menutrans &Close<Tab>^Wc		닫기(&C)<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	다른\ 창\ 닫기(&O)<Tab>^Wo

menutrans Move\ &To			이동(&T)
menutrans &Top<Tab>^WK			꼭대기(&T)<Tab>^WK
menutrans &Bottom<Tab>^WJ		바닥(&B)<Tab>^WJ
menutrans &Left\ side<Tab>^WH		왼쪽(&L)<Tab>^WH
menutrans &Right\ side<Tab>^WL		오른쪽(&R)<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR		위로\ 회전(&U)<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		아래로\ 회전(&D)<Tab>^Wr

menutrans &Equal\ Size<Tab>^W=		같은\ 높이로(&E)<Tab>^W=
menutrans &Max\ Height<Tab>^W_		최대\ 높이로(&M)<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		최소\ 높이로(&i)<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		최대\ 넓이로(&W)<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		최소\ 넓이로(&h)<Tab>^W1\|

" The popup menu
menutrans &Undo			취소(&U)
menutrans Cu&t			자르기(&t)
menutrans &Copy			복사(&C)
menutrans &Paste		붙이기(&P)
menutrans &Delete		삭제(&D)
menutrans Select\ Blockwise	사각형\ 선택
menutrans Select\ &Word		단어\ 선택(&W)
menutrans Select\ &Line		라인\ 선택(&L)
menutrans Select\ &Block	구획\ 선택(&B)
menutrans Select\ &All		모두\ 선택(&A)

" The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		파일 열기
    tmenu ToolBar.Save		현재 파일 저장
    tmenu ToolBar.SaveAll	모두 저장
    tmenu ToolBar.Print		인쇄
    tmenu ToolBar.Undo		취소
    tmenu ToolBar.Redo		재실행
    tmenu ToolBar.Cut		자르기
    tmenu ToolBar.Copy		복사
    tmenu ToolBar.Paste		붙이기
    tmenu ToolBar.Find		찾기...
    tmenu ToolBar.FindNext	다음 찾기
    tmenu ToolBar.FindPrev	이전 찾기
    tmenu ToolBar.Replace	바꾸기
    tmenu ToolBar.LoadSesn	세션 읽어오기
    tmenu ToolBar.SaveSesn	세션 저장
    tmenu ToolBar.RunScript	스크립트 실행
    tmenu ToolBar.Make		Make
    tmenu ToolBar.Shell		쉘
    tmenu ToolBar.RunCtags	태그 생성
    tmenu ToolBar.TagJump	태그 점프
    tmenu ToolBar.Help		도움말
    tmenu ToolBar.FindHelp	도움말 찾기...
  endfun
endif

" Syntax menu
menutrans &Syntax		문법(&S)
menutrans Set\ '&syntax'\ only	'syntax'만\ 설정(&s)
menutrans Set\ '&filetype'\ too	'filetype'도\ 설정(&f)
menutrans &Off			끄기(&O)
menutrans &Manual		수동(&M)
menutrans A&utomatic		자동(&u)
menutrans on/off\ for\ &This\ file 이\ 파일만\ 켜기/끄기(&T)

menutrans Co&lor\ test		색\ 시험(&l)
menutrans &Highlight\ test	Highlight\ 시험(&H)
menutrans &Convert\ to\ HTML	HTML로\ 변환(&C)

