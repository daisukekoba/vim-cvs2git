:: Start Vim on a copy of the tutor file.
@echo off

:: Usage: vimtutor [xx], where xx is a language code like "es" or "nl".
:: When an argument is given, it tries loading that tutor. 
:: When this fails or no argument was given, it tries using 'v:lang'
:: When that also fails, it uses the English version.

SET xx=%1

:: Use Vim to copy the tutor, it knows the value of $VIMRUNTIME
FOR %%d in (. %TMP% %TEMP%) DO IF EXIST %%d\nul.ext SET TUTORCOPY=%%d\$tutor$

:: Try making a copy of tutor with gvim.  If gvim cannot be found, try using
:: vim instead.  If vim cannot be found, alert user to check environment and
:: installation.

:: The script tutor.vim tells Vim which file to copy
gvim -u NONE -c "so $VIMRUNTIME/tutor/tutor.vim"
IF ERRORLEVEL 1 GOTO use_vim

:: Start gvim without any .vimrc, set 'nocompatible'
gvim -u NONE -c "set nocp" %TUTORCOPY%

GOTO end

:use_vim
:: The script tutor.vim tells Vim which file to copy
vim -u NONE -c "so $VIMRUNTIME/tutor/tutor.vim"
IF ERRORLEVEL 1 GOTO no_executable

:: Start vim without any .vimrc, set 'nocompatible'
vim -u NONE -c "set nocp" %TUTORCOPY%

GOTO end

:no_executable
ECHO.
ECHO.
ECHO No vim or gvim found in current directory or PATH.
ECHO Check your installation or re-run install.exe 

:end
:: remove the copy of the tutor
IF EXIST %TUTORCOPY% DEL %TUTORCOPY%
