" Vim syntax file
" Language:     fstab file
" Maintaner:    Radu Dineiu <littledragon@altern.org>
" URL:          http://ld.yi.org/vim/fstab.vim
" Last Change:  2003 Jan 24
" Version:      0.5

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

" General
syn cluster fsGeneralCluster contains=fsComment
syn match fsComment /\s*#.*/
syn match fsOperator /[,=]/

" Device
syn cluster fsDeviceCluster contains=fsOperator,fsDeviceKeyword,fsDeviceError
syn match fsDeviceError /\%([^a-zA-Z0-9_\/#@]\|^\w\{-}\ze\W\)/ contained
syn keyword fsDeviceKeyword contained none proc linproc
syn keyword fsDeviceKeyword contained LABEL nextgroup=fsDeviceLabel
syn match fsDeviceLabel contained /=[^ \t]\+/hs=s+1 contains=fsOperator

" Mount Point
syn cluster fsMountPointCluster contains=fsMountPointKeyword,fsMountPointError
syn match fsMountPointError /\%([^ \ta-zA-Z0-9_\/#@]\|\s\+\zs\w\{-}\ze\s\)/ contained
syn keyword fsMountPointKeyword contained none swap

" Type
syn cluster fsTypeCluster contains=fsTypeKeyword,fsTypeError
syn match fsTypeError /\s\+\zs\w\+/ contained
syn keyword fsTypeKeyword contained adfs affs auto autofs cd9660 coda cramfs devfs devpts efs ext2 ext3 fdesc hfs hpfs iso9660 kernfs linprocfs mfs minix msdos ncpfs nfs ntfs nwfs null portal proc procfs qnx4 reiserfs romfs smbfs std sysv swap tmpfs udf ufs umap umsdos union vfat xfs

" Options
" -------
" Options: General
syn cluster fsOptionsCluster contains=fsOperator,fsOptionsGeneral,fsOptionsKeywords,fsTypeError
syn match fsOptionsNumber /\d\+/
syn match fsOptionsString /\w\+/
syn keyword fsOptionsYesNo yes no
syn cluster fsOptionsCheckCluster contains=fsOptionsExt2Check,fsOptionsFatCheck
syn keyword fsOptionsSize 512 1024 2048
syn keyword fsOptionsGeneral async atime auto current defaults dev exec force fstab noatime noauto noclusterr noclusterw nodev noexec nosuid nosymfollow nouser owner ro rdonly rw rq sw xx suid suiddir sync kudzu union update user
syn match fsOptionsGeneral /_netdev/

" Options: adfs
syn match fsOptionsKeywords contained /\%([ug]id\|o\%(wn\|th\)mask\)=/ nextgroup=fsOptionsNumber

" Options: affs
syn match fsOptionsKeywords contained /\%(set[ug]id\|mode\|reserved\)=/ nextgroup=fsOptionsNumber
syn match fsOptionsKeywords contained /\%(prefix\|volume\|root\)=/ nextgroup=fsOptionsString
syn match fsOptionsKeywords contained /bs=/ nextgroup=fsOptionsSize
syn keyword fsOptionsKeywords contained protect usemp verbose

" Options: cd9660
syn keyword fsOptionsKeywords contained extatt gens norrip nostrictjoilet

" Options: devpts
" -- everything already defined

" Options: ext2
syn match fsOptionsKeywords contained /check=*/ nextgroup=@fsOptionsCheckCluster
syn match fsOptionsKeywords contained /errors=/ nextgroup=fsOptionsExt2Errors
syn match fsOptionsKeywords contained /\%(res[gu]id\|sb\)=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsExt2Check contained none normal strict
syn keyword fsOptionsExt2Errors contained continue panic
syn match fsOptionsExt2Errors contained /remount-ro/
syn keyword fsOptionsKeywords contained bsddf minixdf debug grpid bsdgroups nocheck nogrpid sysvgroups nouid32

" Options: ext3
syn match fsOptionsKeywords contained /journal=/ nextgroup=fsOptionsExt3Journal
syn match fsOptionsKeywords contained /data=/ nextgroup=fsOptionsExt3Data
syn keyword fsOptionsExt3Journal contained update inum
syn keyword fsOptionsExt3Data contained journal ordered writeback
syn keyword fsOptionsKeywords contained noload

" Options: fat
syn match fsOptionsKeywords contained /blocksize=/ nextgroup=fsOptionsSize
syn match fsOptionsKeywords contained /\%(umask\|codepage\)=/ nextgroup=fsOptionsNumber
syn match fsOptionsKeywords contained /\%(cvf_\%(format\|option\)\|iocharset\)=/ nextgroup=fsOptionsString
syn match fsOptionsKeywords contained /check=/ nextgroup=@fsOptionsCheckCluster
syn match fsOptionsKeywords contained /conv=*/ nextgroup=fsOptionsConv
syn match fsOptionsKeywords contained /fat=/ nextgroup=fsOptionsFatType
syn match fsOptionsKeywords contained /dotsOK=/ nextgroup=fsOptionsYesNo
syn keyword fsOptionsFatCheck contained r n s relaxed normal strict
syn keyword fsOptionsConv contained b t a binary text auto
syn keyword fsOptionsFatType contained 12 16 32
syn keyword fsOptionsKeywords contained quiet sys_immutable showexec dots nodots

" Options: hpfs
syn match fsOptionsKeywords contained /case=/ nextgroup=fsOptionsHpfsCase
syn keyword fsOptionsHpfsCase contained lower asis

" Options: iso9660
syn match fsOptionsKeywords contained /map=/ nextgroup=fsOptionsIsoMap
syn match fsOptionsKeywords contained /block=/ nextgroup=fsOptionsSize
syn match fsOptionsKeywords contained /\%(session\|sbsector\)=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsIsoMap contained n o a normal off acorn
syn keyword fsOptionsKeywords contained norock nojoilet unhide cruft
syn keyword fsOptionsConv contained m mtext

" Options: nfs
syn match fsOptionsKeywords contained /\%(rsize\|wsize\|timeo\|retrans\|acregmin\|acregmax\|acdirmin\|acdirmax\|actimeo\|retry\|port\|mountport\|mounthost\|mountprog\|mountvers\|nfsprog\|nfsvers\|namelen\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained bg fg soft hard intr cto ac tcp udp lock nobg nofg nosoft nohard nointr noposix nocto noac notcp noudp nolock

" Options: ntfs
syn match fsOptionsKeywords contained /\%(posix=*\|uni_xlate=\)/ nextgroup=fsOptionsNumber
syn keyword fsOptionsKeywords contained utf8

" Options: proc
" -- everything already defined

" Options: reiserfs
syn match fsOptionsKeywords contained /hash=/ nextgroup=fsOptionsReiserHash
syn match fsOptionsKeywords contained /resize=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsReiserHash contained rupasov tea r5 detect
syn keyword fsOptionsKeywords contained hashed_relocation noborder nolog notail no_unhashed_relocation replayonly

" Options: udf
syn match fsOptionsKeywords contained /\%(anchor\|partition\|lastblock\|fileset\|rootdir\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained unhide undelete strict novrs

" Options: ufs
syn match fsOptionsKeywords contained /ufstype=/ nextgroup=fsOptionsUfsType
syn match fsOptionsKeywords contained /onerror=/ nextgroup=fsOptionsUfsError
syn keyword fsOptionsUfsType contained old 44bsd sun sunx86 nextstep openstep
syn match fsOptionsUfsType contained /nextstep-cd/
syn keyword fsOptionsUfsError contained panic lock umount repair

" Options: vfat
syn keyword fsOptionsKeywords contained nonumtail

" Options: xfs
syn match fsOptionsKeywords contained /\%(biosize\|logbufs\|logbsize\|logdev\|rtdev\|sunit\|swidth\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained dmapi xdsm noalign noatime norecovery osyncisdsync quota usrquota uquoenforce grpquota gquoenforce

" Frequency / Pass No.
syn cluster fsFreqPassCluster contains=fsFreqPassNumber,fsFreqPassError
syn match fsFreqPassError /\s\+\zs\%(\D.*\|\S.*\|\d\+\s\+[^012]\)\ze/ contained
syn match fsFreqPassNumber /\d\+\s\+[012]\s*/ contained

" Groups
syn match fsDevice /^\s*\zs.\{-1,}\s/me=e-1 nextgroup=fsMountPoint contains=@fsDeviceCluster,@fsGeneralCluster
syn match fsMountPoint /\s\+.\{-}\s/me=e-1 nextgroup=fsType contains=@fsMountPointCluster,@fsGeneralCluster contained
syn match fsType /\s\+.\{-}\s/me=e-1 nextgroup=fsOptions contains=@fsTypeCluster,@fsGeneralCluster contained
syn match fsOptions /\s\+.\{-}\s/me=e-1 nextgroup=fsFreqPass contains=@fsOptionsCluster,@fsGeneralCluster contained
syn match fsFreqPass /\s\+.\{-}$/ contains=@fsFreqPassCluster,@fsGeneralCluster contained

" Whole line comments
syn match fsCommentLine /^#.*$/

if version >= 508 || !exists("did_config_syntax_inits")
	if version < 508
		let did_config_syntax_inits = 1
		command! -nargs=+ HiLink hi link <args>
	else
		command! -nargs=+ HiLink hi def link <args>
	endif
	
	HiLink fsOperator Operator
	HiLink fsComment Comment
	HiLink fsCommentLine Comment

	HiLink fsTypeKeyword Type
	HiLink fsDeviceKeyword Identifier
	HiLink fsDeviceLabel String
	HiLink fsFreqPassNumber Number

	HiLink fsTypeError Error
	HiLink fsDeviceError Error
	HiLink fsMountPointError Error
	HiLink fsMountPointKeyword Keyword
	HiLink fsFreqPassError Error

	HiLink fsOptionsGeneral Type
	HiLink fsOptionsKeywords Keyword
	HiLink fsOptionsNumber Number
	HiLink fsOptionsString String
	HiLink fsOptionsSize Number
	HiLink fsOptionsExt2Check String
	HiLink fsOptionsExt2Errors String
	HiLink fsOptionsExt3Journal String
	HiLink fsOptionsExt3Data String
	HiLink fsOptionsFatCheck String
	HiLink fsOptionsConv String
	HiLink fsOptionsFatType Number
	HiLink fsOptionsYesNo String
	HiLink fsOptionsHpfsCase String
	HiLink fsOptionsIsoMap String
	HiLink fsOptionsReiserHash String
	HiLink fsOptionsUfsType String
	HiLink fsOptionsUfsError String

	delcommand HiLink
endif

let b:current_syntax = "fstab"

" vim: ts=8 ft=vim
