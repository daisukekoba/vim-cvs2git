/* vi:set ts=8 sts=4 sw=4:
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * This is NOT the original regular expression code as written by Henry
 * Spencer.  This code has been modified specifically for use with Vim, and
 * should not be used apart from compiling Vim.  If you want a good regular
 * expression library, get the original code.
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 */

#ifndef _REGEXP_H
#define _REGEXP_H

/*
 * The number of sub-matches is limited to 10.
 * The first one (index 0) is the whole match, referenced with "\0".
 * The second one (index 1) is the first sub-match, referenced with "\1".
 * This goes up to the tenth (index 9), referenced with "\9".
 */
#define NSUBEXP  10

/*
 * Structure returned by vim_regcomp() to pass on to vim_regexec().
 * These fields are only to be used in regexp.c!
 * See regep.c for an explanation.
 */
typedef struct
{
    int			regstart;
    char_u		reganch;
    char_u		*regmust;
    int			regmlen;
    unsigned		regflags;
    char_u		reghasz;
    char_u		program[1];		/* actually longer.. */
} regprog_t;

/*
 * Structure to be used for single-line matching.
 * Sub-match "no" starts at "startp[no]" and ends just before "endp[no]".
 * When there is no match, the pointer is NULL.
 */
typedef struct
{
    regprog_t		*regprog;
    char_u		*startp[NSUBEXP];
    char_u		*endp[NSUBEXP];
    int			rm_ic;
} regmatch_t;

/*
 * Structure to be used for multi-line matching.
 * Sub-match "no" starts in line "startpos[no].lnum" column "startpos[no].col"
 * and ends in line "endpos[no].lnum" just before column "endpos[no].col".
 * The line numbers are relative to the first line, thus startpos[0].lnum is
 * always 0.
 * When there is no match, the line number is -1.
 */
typedef struct
{
    regprog_t		*regprog;
    pos_t		startpos[NSUBEXP];
    pos_t		endpos[NSUBEXP];
    int			rmm_ic;
} regmmatch_t;

/*
 * Structure used to store external references: "\z\(\)" to "\z\1".
 * Use a reference count to avoid the need to copy this around.  When it goes
 * from 1 to zero the matches need to be freed.
 */
typedef struct
{
    short		refcnt;
    char_u		*matches[NSUBEXP];
} reg_extmatch_t;

#endif	/* _REGEXP_H */
