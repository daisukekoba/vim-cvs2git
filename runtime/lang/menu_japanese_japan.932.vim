" vi:set ts=8 sts=8 sw=8 tw=0:
"
" Menu Translations:	Japanese (for Windows)
" Translated By: 	Muraoka Taro  <koron@tka.att.ne.jp>
" Last Change:		21:36:27 30-Apr-2001.

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding cp932

" Help menu
menutrans &Help			ヘルプ(&H)
menutrans &Overview<Tab><F1>	概略(&O)<Tab><F1>
menutrans &How-to\ links	&How-toリンク
menutrans &Credits		クレジット(&C)
menutrans Co&pying		著作権情報(&P)
menutrans &Version		バージョン情報(&V)
" menutrans &About		&About

" File menu
menutrans &File				ファイル(&F)
menutrans &Open\.\.\.<Tab>:e		開く(&O)\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	分割して開く(&L)\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		新規作成(&N)<Tab>:enew
menutrans &Close<Tab>:q			閉じる(&C)<Tab>:q
menutrans &Save<Tab>:w			保存(&S)<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	名前を付けて保存(&A)\.\.\.<Tab>:sav
menutrans Show\ &Diff\ with\.\.\.	差分表示(&D)\.\.\.
menutrans Show\ &Patched\ by\.\.\.	パッチ結果を表示(&P)\.\.\.
menutrans &Print			印刷(&P)
menutrans Sa&ve-Exit<Tab>:wqa		保存して終了(&V)<Tab>:wqa
menutrans E&xit<Tab>:qa			終了(&X)<Tab>:qa

" Edit menu
menutrans &Edit				編集(&E)
menutrans &Undo<Tab>u			取り消す(&U)<Tab>u
menutrans &Redo<Tab>^R			もう一度やる(&R)<Tab>^R
menutrans Rep&eat<Tab>\.		繰り返す(&T)<Tab>\.
menutrans Cu&t<Tab>"+x			切り取り(&T)<Tab>"+x
menutrans &Copy<Tab>"+y			コピー(&C)<Tab>"+y
menutrans &Paste<Tab>"+P		貼り付け(&P)<Tab>"+P
menutrans Put\ &Before<Tab>[p		前に貼る(&B)<Tab>[p
menutrans Put\ &After<Tab>]p		後に貼る(&A)<Tab>]p
menutrans &Delete<Tab>x			消す(&D)<Tab>x
menutrans &Select\ all<Tab>ggVG		全て選択(&S)<Tab>ggvG
menutrans &Find\.\.\.			検索(&F)\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.	置換(&E)\.\.\.
"menutrans Options\.\.\.			オプション(&O)\.\.\.
menutrans Settings\ &Window		設定ウィンドウ(&W)

" Edit/Global Settings
menutrans Global\ Settings		全体設定
menutrans Toggle\ Pattern\ Highlight<Tab>:set\ hls!
	\	パターン強調切替<Tab>:set\ hls!
menutrans Toggle\ ignore-case<Tab>:set\ ic!
	\	大小文字区別切替<Tab>:set\ ic!
menutrans Toggle\ showmatch<Tab>:set\ sm!
	\	マッチ表示切替<Tab>:set\ sm!
menutrans Context\ lines		カーソル周辺行数
menutrans Virtual\ Edit			仮想編集
menutrans Never				無効
menutrans Block\ Selection		ブロック選択時
menutrans Insert\ mode			挿入モード時
menutrans Block\ and\ Insert		ブロック/挿入モード時
menutrans Always			常時
menutrans Toggle\ Insert\ mode<Tab>:set\ im!
	\	挿入(初心者)モード切替<Tab>:set\ im!
menutrans Search\ Path\.\.\.		検索パス\.\.\.
menutrans Tag\ Files\.\.\.		タグファイル\.\.\.

" GUI options
menutrans Toggle\ Toolbar		ツールバー表示切替
menutrans Toggle\ Bottom\ Scrollbar	スクロールバー(下)表示切替
menutrans Toggle\ Left\ Scrollbar	スクロールバー(左)表示切替
menutrans Toggle\ Right\ Scrolbar	スクロールバー(右)表示切替

" Edit/File Settings

" Boolean options
menutrans File\ Settings		ファイル設定
menutrans Toggle\ Line\ Numbering<Tab>:set\ nu!
	\	行番号表示切替<Tab>:set\ nu!
menutrans Toggle\ List\ Mode<Tab>:set\ list!
	\ リストモード切替<Tab>:set\ list!
menutrans Toggle\ Line\ Wrap<Tab>:set\ wrap!		
	\	行折返し切替<Tab>:set\ wrap!
menutrans Toggle\ Wrap\ at\ word<Tab>:set\ lbr!
	\	単語折返し切替<Tab>:set\ lbr!
menutrans Toggle\ Search\ Patn\ Highl<Tab>:set\ hls!
	\	強調検索切替<Tab>:set\ hls!
menutrans Toggle\ expand-tab<Tab>:set\ et!
	\	タブ展開切替<Tab>:set\ et!
menutrans Toggle\ auto-indent<Tab>:set\ ai!
	\	自動字下げ切替<Tab>:set\ ai!
menutrans Toggle\ C-indenting<Tab>:set\ cin!
	\	C言語字下げ切替<Tab>:set\ cin!

" other options
menutrans Shiftwidth			シフト幅
menutrans Soft\ Tabstop			ソフトウェアタブ幅
menutrans Text\ Width\.\.\.		テキスト幅\.\.\.
menutrans File\ Format\.\.\.		改行記号選択\.\.\.
menutrans Keymap			キーマップ
menutrans None				なし

" Programming menu
menutrans &Tools			ツール(&T)
menutrans &Jump\ to\ this\ tag<Tab>g^]	タグジャンプ(&J)<Tab>g^]
menutrans Jump\ &back<Tab>^T		戻る(&B)<Tab>^T
menutrans Build\ &Tags\ File		タグファイル作成
menutrans &Make<Tab>:make		メイク(&M)<Tab>:make
menutrans &List\ Errors<Tab>:cl		エラーリスト(&E)<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	メッセージリスト(&I)<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		次のエラーへ(&N)<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	前のエラーへ(&P)<Tab>:cp
menutrans &Older\ List<Tab>:cold	古いリスト(&O)<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	新しいリスト(&E)<Tab>:cnew
menutrans Error\ &Window<Tab>:cwin	エラーウィンドウ表示(&W)<Tab>:cwin	
menutrans Convert\ to\ HEX<Tab>:%!xxd	HEXへ変換<Tab>:%!xxd
menutrans Convert\ back<Tab>:%!xxd\ -r	HEXから逆変換<Tab>%!xxd\ -r

" Tools.Fold Menu
menutrans &Folding			折畳み(&F)
" open close folds 
menutrans &Enable/Disable\ folds<Tab>zi	有効/無効切替(&E)<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv	カーソル行を表示(&V)<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	カーソル行だけを表示(&W)<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm	折畳みを閉じる(&L)<Tab>zm
menutrans &Close\ all\ folds<Tab>zM	全折畳みを閉じる(&C)<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr	折畳みを開く(&P)<Tab>zr
menutrans &Open\ all\ folds<Tab>zR	全折畳みを開く(&O)<Tab>zR
" fold method
menutrans Fold\ Met&hod			折畳み方法(&H)
menutrans M&anual			手動
menutrans I&ndent			インデント
menutrans E&xpression			式評価
menutrans S&yntax			シンタックス
menutrans &Diff				差分
menutrans Ma&rker			マーカー
" create and delete folds
menutrans Create\ &Fold<Tab>zf		折畳み作成(&F)<Tab>zf
menutrans &Delete\ Fold<Tab>zd		折畳み削除(&D)<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD	全折畳み削除(&A)<Tab>zD
" moving around in folds
menutrans Fold\ column\ &width		折畳みカラム幅

menutrans &Update		更新(&U)
menutrans &Get\ Block		ブロック抽出(&G)
menutrans &Put\ Block		ブロック適用(&P)

" Names for buffer menu.
menutrans &Buffers		バッファ(&B)
menutrans &Refresh\ menu	メニュー再読込(&R)
menutrans &Delete		削除(&D)
menutrans A&lternate		裏へ切替(&L)
menutrans &Next			次のバッファ(&N)
menutrans &Previous		前のバッファ(&P)
menutrans [No\ File]		[新規ファイル]

" Window menu
menutrans &Window			ウィンドウ(&W)
menutrans &New<Tab>^Wn			新規作成(&N)<Tab>^Wn
menutrans S&plit<Tab>^Ws		分割(&P)<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	裏バッファへ分割(&L)<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv	垂直分割(&P)<Tab>^Wv
menutrans Split\ File\ E&xplorer	ファイルエクスプローラ(&x)
"menutrans File\ E&xplorer		ファイルエクスプローラ(&x)
menutrans &Close<Tab>^Wc		閉じる(&C)<Tab>^Wc
menutrans Move\ &To			移動(&T)
menutrans &Top<Tab>^WK			先頭(&T)
menutrans &Bottom<Tab>^WJ		末尾(&B)
menutrans &Left\ side<Tab>^WH		左(&L)
menutrans &Right\ side<Tab>^WL		右(&R)
menutrans Close\ &Other(s)<Tab>^Wo	他を閉じる(&O)<Tab>^Wo
menutrans Ne&xt<Tab>^Ww			次へ(&X)<Tab>^Ww
menutrans P&revious<Tab>^WW		前へ(&R)<Tab>^WW
menutrans &Equal\ Size<Tab>^W=	同じ高さに(&E)<Tab>^W=
menutrans &Max\ Height<Tab>^W_		最大高に(&M)<Tab>^W
menutrans M&in\ Height<Tab>^W1_		最小高に(&i)<Tab>^W1_
menutrans Max\ Width<Tab>^W\|		最大幅に<Tab>^W\|
menutrans Min\ Width<Tab>^W1\|		最小幅に<Tab>^W1\|
menutrans Rotate\ &Up<Tab>^WR		上にローテーション(&U)<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		下にローテーション(&D)<Tab>^Wr
menutrans Select\ &Font\.\.\.		フォント設定(&F)\.\.\.

" The popup menu
menutrans &Undo			取り消す(&U)
menutrans Cu&t			切り取り(&T)
menutrans &Copy			コピー(&C)
menutrans &Paste		貼り付け(&P)
menutrans &Delete		削除(&D)
menutrans Select\ Blockwise 	矩形ブロック選択
menutrans Select\ &Word		単語選択(&W)
menutrans Select\ &Line		行選択(&L)
menutrans Select\ &Block	ブロック選択
menutrans Select\ &All		すべて選択
 
" The GUI toolbar
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		ファイルを開く
    tmenu ToolBar.Save		現在のファイルを保存
    tmenu ToolBar.SaveAll	すべてのファイルを保存
    tmenu ToolBar.Print		印刷
    tmenu ToolBar.Undo		取り消し
    tmenu ToolBar.Redo		もう一度やる
    tmenu ToolBar.Cut		クリップボードへ切り取り
    tmenu ToolBar.Copy		クリップボードへコピー
    tmenu ToolBar.Paste		クリップボードから貼り付け
    tmenu ToolBar.Find		検索...
    tmenu ToolBar.FindNext	次を検索
    tmenu ToolBar.FindPrev	前を検索
    tmenu ToolBar.Replace	置換...
    if 0	" disabled; These are in the Windows menu
      tmenu ToolBar.New		新規ウィンドウ作成
      tmenu ToolBar.WinSplit	ウィンドウ分割
      tmenu ToolBar.WinMax	ウィンドウ最大化
      tmenu ToolBar.WinMin	ウィンドウ最小化
      tmenu ToolBar.WinClose	ウィンドウを閉じる
    endif
    tmenu ToolBar.LoadSesn	セッション読込
    tmenu ToolBar.SaveSesn	セッション保存
    tmenu ToolBar.RunScript	Vimスクリプト実行
    tmenu ToolBar.Make		プロジェクトをMake
    tmenu ToolBar.Shell		シェルを開く
    tmenu ToolBar.RunCtags	tags作成
    tmenu ToolBar.TagJump	タグジャンプ
    tmenu ToolBar.Help		Vimヘルプ
    tmenu ToolBar.FindHelp	Vimヘルプ検索
  endfun
endif

" Syntax menu
menutrans &Syntax		シンタックス(&S)
menutrans Set\ 'syntax'\ only	'syntax'だけ設定
menutrans Set\ 'filetype'\ too	'filetype'も設定
menutrans &Off			無効化(&O)
menutrans &Manual		手動設定(&M)
menutrans A&utomatic		自動設定(&U)
menutrans o&n\ (this\ file)	オン(&O)\ (このファイル)
menutrans o&ff\ (this\ file)	オフ(&F)\ (このファイル)
menutrans Co&lor\ test		カラーテスト(&L)
menutrans &Highlight\ test	ハイライトテスト(&H)
menutrans &Convert\ to\ HTML	HTMLへコンバート(&C)
