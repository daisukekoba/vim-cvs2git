/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 * Multibyte extensions partly by Sung-Hoon Baek
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * mbyte.c: Code specifically for handling multi-byte characters.
 *
 * The encoding used in the core is set with 'encoding'.  When 'encoding' is
 * changed, the following four variables are set (for speed).
 * Currently these types of character encodings are supported:
 *
 * "enc_dbcs"	    When non-zero it tells the type of double byte character
 *		    encoding (Chinese, Korean, Japanese, etc.).
 *		    The cell width on the display is equal to the number of
 *		    bytes.  (exception: DBCS_JPNU with first byte 0x8e)
 *		    Recognizing the first or second byte is difficult, it
 *		    requires checking a byte sequence from the start.
 * "enc_utf8"	    When TRUE use Unicode characters in UTF-8 encoding.
 *		    The cell width on the display needs to be determined from
 *		    the character value.
 *		    Recognizing bytes is easy: 0xxx.xxxx is a single-byte
 *		    char, 10xx.xxxx is a trailing byte, 11xx.xxxx is a leading
 *		    byte of a multi-byte character.
 *		    To make things complicated, up to two composing characters
 *		    are allowed.  These are drawn on top of the first char.
 *		    For most editing the sequence of bytes with composing
 *		    characters included is considered to be one character.
 * "enc_unicode"    When 2 use 16-bit Unicode characters (or UTF-16).
 *		    When 4 use 32-but Unicode characters.
 *		    Internally characters are stored in UTF-8 encoding to
 *		    avoid NUL bytes.  Conversion happens when doing I/O.
 *		    "enc_utf8" will also be TRUE.
 *
 * "has_mbyte" is set when "enc_dbcs" or "enc_utf8" is non-zero.
 *
 * If none of these is TRUE, 8-bit bytes are used for a character.  The
 * encoding isn't currently specified (TODO).
 *
 * 'encoding' specifies the encoding used in the core.  This is in registers,
 * text manipulation, buffers, etc.  Conversion has to be done when characters
 * in another encoding are received or send:
 *
 *		       clipboard
 *			   ^
 *			   | (2)
 *			   V
 *		   +---------------+
 *	      (1)  |		   | (3)
 *  keyboard ----->|	 core	   |-----> display
 *		   |		   |
 *		   +---------------+
 *			   ^
 *			   | (4)
 *			   V
 *			 file
 *
 * (1) Typed characters arrive in the current locale.  Conversion is to be
 *     done when 'encoding' is different from 'termencoding'.
 * (2) Text will be made available with the encoding specified with
 *     'encoding'.  If this is not sufficient, system-specific conversion
 *     might be required.
 * (3) For the GUI the correct font must be selected, no conversion done.
 *     Otherwise, conversion is to be done when 'encoding' differs from
 *     'termencoding'.
 * (4) The encoding of the file is specified with 'fileencoding'.  Conversion
 *     is to be done when it's different from 'encoding'.
 *
 * The viminfo file is a special case: Only text is converted, not file names.
 * Vim scripts may contain an ":encoding" command.  This has an effect for
 * some commands, like ":menutrans"
 */

#include "vim.h"
#if defined(WIN3264) || defined(WIN32UNIX)
# include <windows.h>
# ifndef __MINGW32__
#  include <winnls.h>
# endif
#endif
#ifdef FEAT_GUI_X11
# include <X11/Intrinsic.h>
#endif
#ifdef X_LOCALE
#include <X11/Xlocale.h>
#endif

#if defined(FEAT_MBYTE) || defined(PROTO)

static int dbcs_ptr2len_check __ARGS((char_u *p));
static int dbcs_char2cells __ARGS((int c));
static int dbcs_char2len __ARGS((int c));
static int dbcs_char2bytes __ARGS((int c, char_u *buf));
static int dbcs_ptr2char __ARGS((char_u *p));
static int enc_alias_search __ARGS((char_u *name));

/* Lookup table to quickly get the length in bytes of a UTF-8 character from
 * the first byte of a UTF-8 string.  Bytes which are illegal when used as the
 * first byte have a one, because these will be used separately. */
static char utf8len_tab[256] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /*bogus*/
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /*bogus*/
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1,
};

/*
 * Canonical encoding names and their properties.
 * "iso-8859-n" is handled by enc_canonize() directly.
 */
static struct
{   char *name;		int prop;		int codepage;}
enc_canon_table[] =
{
#define IDX_LATIN_1	0
    {"latin1",		ENC_8BIT + ENC_LATIN1,	0},
#define IDX_ISO_2	1
    {"iso-8859-2",	ENC_8BIT,		0},
#define IDX_ISO_3	2
    {"iso-8859-3",	ENC_8BIT,		0},
#define IDX_ISO_4	3
    {"iso-8859-4",	ENC_8BIT,		0},
#define IDX_ISO_5	4
    {"iso-8859-5",	ENC_8BIT,		0},
#define IDX_ISO_6	5
    {"iso-8859-6",	ENC_8BIT,		0},
#define IDX_ISO_7	6
    {"iso-8859-7",	ENC_8BIT,		0},
#define IDX_CP1255	7
    {"cp1255",		ENC_8BIT,		0}, /* close to iso-8859-8 */
#define IDX_ISO_8	8
    {"iso-8859-8",	ENC_8BIT,		0},
#define IDX_ISO_9	9
    {"iso-8859-9",	ENC_8BIT,		0},
#define IDX_ISO_10	10
    {"iso-8859-10",	ENC_8BIT,		0},
#define IDX_ISO_11	11
    {"iso-8859-11",	ENC_8BIT,		0},
#define IDX_ISO_13	12
    {"iso-8859-13",	ENC_8BIT,		0},
#define IDX_ISO_14	13
    {"iso-8859-14",	ENC_8BIT,		0},
#define IDX_ISO_15	14
    {"iso-8859-15",	ENC_8BIT,		0},
#define IDX_KOI8_R	15
    {"koi8-r",		ENC_8BIT,		0},
#define IDX_KOI8_U	16
    {"koi8-u",		ENC_8BIT,		0},
#define IDX_UTF8	17
    {"utf-8",		ENC_UNICODE,		0},
#define IDX_UCS2	18
    {"ucs-2",		ENC_UNICODE + ENC_ENDIAN_B + ENC_2BYTE, 0},
#define IDX_UCS2LE	19
    {"ucs-2le",		ENC_UNICODE + ENC_ENDIAN_L + ENC_2BYTE, 0},
#define IDX_UTF16	20
    {"utf-16",		ENC_UNICODE + ENC_ENDIAN_B + ENC_2WORD, 0},
#define IDX_UTF16LE	21
    {"utf-16le",	ENC_UNICODE + ENC_ENDIAN_L + ENC_2WORD, 0},
#define IDX_UCS4	22
    {"ucs-4",		ENC_UNICODE + ENC_ENDIAN_B + ENC_4BYTE, 0},
#define IDX_UCS4LE	23
    {"ucs-4le",		ENC_UNICODE + ENC_ENDIAN_L + ENC_4BYTE, 0},
#define IDX_DEBUG	24
    {"debug",		ENC_DBCS,		DBCS_DEBUG},
#define IDX_CP932	25
    {"cp932",		ENC_DBCS,		DBCS_JPN},
#define IDX_CP949	26
    {"cp949",		ENC_DBCS,		DBCS_KOR},
#define IDX_CP936	27
    {"cp936",		ENC_DBCS,		DBCS_CHS},
#define IDX_CP950	28
    {"cp950",		ENC_DBCS,		DBCS_CHT},
#define IDX_EUC_JP	29
    {"euc-jp",		ENC_DBCS,		DBCS_JPNU},
#define IDX_SJIS	30
    {"sjis",		ENC_DBCS,		DBCS_JPN},
#define IDX_EUC_KR	31
    {"euc-kr",		ENC_DBCS,		DBCS_KORU},
#define IDX_EUC_CN	32
    {"euc-cn",		ENC_DBCS,		DBCS_CHSU},
#define IDX_EUC_TW	33
    {"euc-tw",		ENC_DBCS,		DBCS_CHTU},
#define IDX_BIG5	34
    {"big5",		ENC_DBCS,		DBCS_CHT},
#define IDX_COUNT	35
};

/*
 * Aliases for encoding names.
 */
static struct
{   char *name;		int canon;}
enc_alias_table[] =
{
    {"ansi",		IDX_LATIN_1},
    {"iso-8859-1",	IDX_LATIN_1},
    {"latin2",		IDX_ISO_2},
    {"latin3",		IDX_ISO_3},
    {"latin4",		IDX_ISO_4},
    {"cyrillic",	IDX_ISO_5},
    {"arabic",		IDX_ISO_6},
    {"greek",		IDX_ISO_7},
#ifdef WIN3264
    {"hebrew",		IDX_CP1255},
#else
    {"hebrew",		IDX_ISO_8},
#endif
    {"latin5",		IDX_ISO_9},
    {"turkish",		IDX_ISO_9}, /* ? */
    {"latin6",		IDX_ISO_10},
    {"nordic",		IDX_ISO_10}, /* ? */
    {"thai",		IDX_ISO_11}, /* ? */
    {"latin7",		IDX_ISO_13},
    {"latin8",		IDX_ISO_14},
    {"latin9",		IDX_ISO_15},
    {"utf8",		IDX_UTF8},
    {"unicode",		IDX_UCS2},
    {"ucs2",		IDX_UCS2},
    {"ucs2be",		IDX_UCS2},
    {"ucs-2be",		IDX_UCS2},
    {"ucs2le",		IDX_UCS2LE},
    {"utf16",		IDX_UTF16},
    {"utf16be",		IDX_UTF16},
    {"utf-16be",	IDX_UTF16},
    {"utf16le",		IDX_UTF16LE},
    {"ucs4",		IDX_UCS4},
    {"ucs4be",		IDX_UCS4},
    {"ucs-4be",		IDX_UCS4},
    {"ucs4le",		IDX_UCS4LE},
    {"932",		IDX_CP932},
    {"949",		IDX_CP949},
    {"936",		IDX_CP936},
    {"950",		IDX_CP950},
    {"eucjp",		IDX_EUC_JP},
    {"unix-jis",	IDX_EUC_JP},
    {"ujis",		IDX_EUC_JP},
    {"shift-jis",	IDX_SJIS},
    {"euckr",		IDX_EUC_KR},
    {"5601",		IDX_EUC_KR},	/* Sun: KS C 5601 */
    {"euccn",		IDX_EUC_CN},
    {"gb2312",		IDX_EUC_CN},
    {"euctw",		IDX_EUC_TW},
#if defined(WIN3264) || defined(WIN32UNIX)
    {"japan",		IDX_CP932},
    {"korea",		IDX_CP949},
    {"prc",		IDX_CP936},
    {"chinese",		IDX_CP936},
    {"taiwan",		IDX_CP950},
    {"big5",		IDX_CP950},
#else
    {"japan",		IDX_EUC_JP},
    {"korea",		IDX_EUC_KR},
    {"prc",		IDX_EUC_CN},
    {"chinese",		IDX_EUC_CN},
    {"taiwan",		IDX_EUC_TW},
    {"cp950",		IDX_BIG5},
    {"950",		IDX_BIG5},
#endif
    {NULL,		0}
};

/*
 * Find encoding "name" in the list of canonical encoding names.
 * Returns -1 if not found.
 */
    static int
enc_canon_search(name)
    char_u	*name;
{
    int		i;

    for (i = 0; i < IDX_COUNT; ++i)
	if (STRCMP(name, enc_canon_table[i].name) == 0)
	    return i;
    return -1;
}

/*
 * Find canonical encoding "name" in the list and return its properties.
 * Returns 0 if not found.
 */
    int
enc_canon_props(name)
    char_u	*name;
{
    int		i;

    i = enc_canon_search(name);
    if (i >= 0)
	return enc_canon_table[i].prop;
#ifdef WIN3264
    if (name[0] == 'c' && name[1] == 'p' && isdigit(name[2]))
    {
	CPINFO	cpinfo;

	/* Get info on this codepage to find out what it is. */
	if (GetCPInfo(atoi(name + 2), &cpinfo) != 0)
	{
	    if (cpinfo.MaxCharSize == 1) /* some single-byte encoding */
		return ENC_8BIT;
	    if (cpinfo.MaxCharSize == 2
		    && (cpinfo.LeadByte[0] != 0 || cpinfo.LeadByte[1] != 0))
		/* must be a DBCS encoding */
		return ENC_DBCS;
	}
	return 0;
    }
#endif
    if (STRNCMP(name, "2byte-", 6) == 0)
	return ENC_DBCS;
    if (STRNCMP(name, "8bit-", 5) == 0 || STRNCMP(name, "iso-8859-", 9) == 0)
	return ENC_8BIT;
    return 0;
}

/*
 * Set up for using multi-byte characters.
 * Called in three cases:
 * - by main() to initialize (p_enc == NULL)
 * - by set_init_1() after 'encoding' was set to its default.
 * - by do_set() when 'encoding' has been set.
 * p_enc must have been passed through enc_canonize() already.
 * Sets the "enc_unicode", "enc_utf8", "enc_dbcs" and "has_mbyte" flags.
 * Fills mb_bytelen_tab[] and returns NULL when there are no problems.
 * When there is something wrong: Returns an error message and doesn't change
 * anything.
 */
    char_u *
mb_init()
{
    int		i;
    int		idx;
    int		n;
    int		enc_dbcs_new = 0;

    if (p_enc == NULL)
    {
	/* Just starting up: set the whole table to one's. */
	for (i = 0; i < 256; ++i)
	    mb_bytelen_tab[i] = 1;
	input_conv.vc_type = CONV_NONE;
	input_conv.vc_factor = 1;
	output_conv.vc_type = CONV_NONE;
#ifdef USE_ICONV
	input_conv.vc_fd = (iconv_t)-1;
	output_conv.vc_fd = (iconv_t)-1;
#endif
	return NULL;
    }

#ifdef WIN3264
    if (p_enc[0] == 'c' && p_enc[1] == 'p' && isdigit(p_enc[2]))
    {
	CPINFO	cpinfo;

	/* Get info on this codepage to find out what it is. */
	if (GetCPInfo(atoi(p_enc + 2), &cpinfo) != 0)
	{
	    if (cpinfo.MaxCharSize == 1)
	    {
		/* some single-byte encoding */
		enc_unicode = 0;
		enc_utf8 = FALSE;
	    }
	    else if (cpinfo.MaxCharSize == 2
		    && (cpinfo.LeadByte[0] != 0 || cpinfo.LeadByte[1] != 0))
	    {
		/* must be a DBCS encoding, check below */
		enc_dbcs_new = atoi(p_enc + 2);
	    }
	    else
		goto codepage_invalid;
	}
	else if (GetLastError() == ERROR_INVALID_PARAMETER)
	{
codepage_invalid:
	    return (char_u *)N_("Not a valid codepage");
	}
    }
#endif
    else if (STRNCMP(p_enc, "8bit-", 5) == 0
	    || STRNCMP(p_enc, "iso-8859-", 9) == 0)
    {
	/* Accept any "8bit-" or "iso-8859-" name. */
	enc_unicode = 0;
	enc_utf8 = FALSE;
    }
    else if (STRNCMP(p_enc, "2byte-", 6) == 0)
    {
#ifdef WIN3264
	/* Windows: accept only valid codepage numbers, check below. */
	if (p_enc[6] != 'c' || p_enc[7] != 'p'
				      || (enc_dbcs_new = atoi(p_enc + 8)) == 0)
	    return e_invarg;
#else
	/* Unix: accept any "2byte-" name, assume current locale. */
	enc_dbcs_new = DBCS_2BYTE;
#endif
    }
    else if ((idx = enc_canon_search(p_enc)) >= 0)
    {
	i = enc_canon_table[idx].prop;
	if (i & ENC_UNICODE)
	{
	    /* Unicode */
	    enc_utf8 = TRUE;
	    if (i & (ENC_2BYTE | ENC_2WORD))
		enc_unicode = 2;
	    else if (i & ENC_4BYTE)
		enc_unicode = 4;
	    else
		enc_unicode = 0;
	}
	else if (i & ENC_DBCS)
	{
	    /* 2byte, handle below */
	    enc_dbcs_new = enc_canon_table[idx].codepage;
	}
	else
	{
	    /* Must be 8-bit. */
	    enc_unicode = 0;
	    enc_utf8 = FALSE;
	}
    }
    else    /* Don't know what encoding this is, reject it. */
	return e_invarg;

    if (enc_dbcs_new != 0)
    {
#ifdef WIN3264
	/* Check if the DBCS code page is OK. */
	if (!IsValidCodePage(enc_dbcs_new))
	    goto codepage_invalid;
#endif
	enc_unicode = 0;
	enc_utf8 = FALSE;
    }
    enc_dbcs = enc_dbcs_new;

#ifdef FEAT_GUI_W32
    /* Check for codepage which is not the current one. */
    is_funky_dbcs = (enc_dbcs != 0 && ((int)GetACP() != enc_dbcs));
#endif

    has_mbyte = (enc_dbcs != 0 || enc_utf8);

    /*
     * Set the function pointers.
     */
    if (enc_utf8)
    {
	mb_ptr2len_check = utfc_ptr2len_check;
	mb_char2len = utf_char2len;
	mb_char2bytes = utf_char2bytes;
	mb_ptr2cells = utf_ptr2cells;
	mb_char2cells = utf_char2cells;
	mb_off2cells = utf_off2cells;
	mb_ptr2char = utf_ptr2char;
	mb_head_off = utf_head_off;
    }
    else if (enc_dbcs != 0)
    {
	mb_ptr2len_check = dbcs_ptr2len_check;
	mb_char2len = dbcs_char2len;
	mb_char2bytes = dbcs_char2bytes;
	mb_ptr2cells = dbcs_ptr2cells;
	mb_char2cells = dbcs_char2cells;
	mb_off2cells = dbcs_off2cells;
	mb_ptr2char = dbcs_ptr2char;
	mb_head_off = dbcs_head_off;
    }
    else
    {
	mb_ptr2len_check = latin_ptr2len_check;
	mb_char2len = latin_char2len;
	mb_char2bytes = latin_char2bytes;
	mb_ptr2cells = latin_ptr2cells;
	mb_char2cells = latin_char2cells;
	mb_off2cells = latin_off2cells;
	mb_ptr2char = latin_ptr2char;
	mb_head_off = latin_head_off;
    }

    /*
     * Fill the mb_bytelen_tab[] for MB_BYTE2LEN().
     */
    for (i = 0; i < 256; ++i)
    {
	/* Our own function to reliably check the lenght of UTF-8 characters,
	 * independent of mblen(). */
	if (enc_utf8)
	    n = utf8len_tab[i];
	else if (enc_dbcs == 0)
	    n = 1;
	else
	{
#if defined(WIN3264) || defined(WIN32UNIX)
	    /* enc_dbcs is set by setting 'fileencoding'.  It becomes a Windows
	     * CodePage identifier, which we can pass directly in to Windows
	     * API */
	    n = IsDBCSLeadByteEx(enc_dbcs, (BYTE)i) ? 2 : 1;
#else
# ifdef MACOS
	    /*
	     * if mblen() is not available, character which MSB is turned on
	     * are treated as leading byte character. (note : This assumption
	     * is not always true.)
	     */
	    n = (i & 0x80) ? 2 : 1;
# else
	    char buf[MB_MAXBYTES];
# ifdef X_LOCALE
	    extern int _Xmblen __ARGS((char *, size_t));
#  ifndef mblen
#   define mblen _Xmblen
#  endif
# endif
	    if (i == NUL)	/* just in case mblen() can't handle "" */
		n = 1;
	    else
	    {
		/*
		 * mblen() should return -1 for invalid (means the leading
		 * multibyte) character.  However there are some platform
		 * where mblen() returns 0 for invalid character.  Therefore,
		 * following condition includes 0.
		 */
		buf[0] = i;
		buf[1] = 0;
#if 0
		if (i >= 0x80)/* TESTING DBCS: 'encoding' != current locale */
#else
		if (mblen(buf, (size_t)1) <= 0)
#endif
		    n = 2;
		else
		    n = 1;
	    }
# endif
#endif
	}

	mb_bytelen_tab[i] = n;
    }

    /* The cell width depends on the type of multi-byte characters. */
    (void)init_chartab();

    /* When enc_utf8 is set or reset, (de)allocate ScreenLinesUC[] */
    screenalloc(FALSE);

    /* When using Unicode, set default for 'fileencodings'. */
    if (enc_utf8 && !option_was_set((char_u *)"fencs"))
	set_string_option_direct((char_u *)"fencs", -1,
				 (char_u *)"ucs-bom,utf-8,latin1", OPT_FREE);

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    /* GNU gettext 0.10.37 supports this feature: set the codeset used for
     * translated messages independently from the current locale. */
    (void)bind_textdomain_codeset(VIMPACKAGE,
					  enc_utf8 ? "utf-8" : (char *)p_enc);
#endif

#ifdef FEAT_AUTOCMD
    /* Fire an autocommand to let people do custom font setup. This must be
     * after Vim has been setup for the new encoding. */
    apply_autocmds(EVENT_ENCODINGCHANGED, NULL, (char_u *)"", FALSE, curbuf);
#endif

    return NULL;
}

/*
 * Return the size of the BOM for the current buffer:
 * 0 - no BOM
 * 2 - UCS-2 or UTF-16 BOM
 * 4 - UCS-4 BOM
 * 3 - UTF-8 BOM
 */
    int
bomb_size()
{
    int n = 0;

    if (curbuf->b_p_bomb && !curbuf->b_p_bin)
    {
	if (*curbuf->b_p_fenc == NUL)
	{
	    if (enc_utf8)
	    {
		if (enc_unicode != 0)
		    n = enc_unicode;
		else
		    n = 3;
	    }
	}
	else if (STRCMP(curbuf->b_p_fenc, "utf-8") == 0)
	    n = 3;
	else if (STRNCMP(curbuf->b_p_fenc, "ucs-2", 5) == 0
		|| STRNCMP(curbuf->b_p_fenc, "utf-16", 6) == 0)
	    n = 2;
	else if (STRNCMP(curbuf->b_p_fenc, "ucs-4", 5) == 0)
	    n = 4;
    }
    return n;
}

/*
 * Get class of pointer:
 * 0 for blank or NUL
 * 1 for punctuation
 * 2 for an (ASCII) word character
 * >2 for other word characters
 */
    int
mb_get_class(p)
    char_u	*p;
{
    if (MB_BYTE2LEN(p[0]) == 1)
    {
	if (p[0] == NUL || vim_iswhite(p[0]))
	    return 0;
	if (vim_iswordc(p[0]))
	    return 2;
	return 1;
    }
    if (enc_dbcs != 0 && p[0] != NUL && p[1] != NUL)
	return dbcs_class(p[0], p[1]);
    if (enc_utf8)
	return utf_class(utf_ptr2char(p));
    return 0;
}

/*
 * Get class of a double-byte character.  This always returns 3 or bigger.
 * TODO: Should return 1 for punctuation.
 */
    int
dbcs_class(lead, trail)
    unsigned	lead;
    unsigned	trail;
{
    switch (enc_dbcs)
    {
	/* please add classfy routine for your language in here */

	case DBCS_JPNU:	/* ? */
	case DBCS_JPN:
	    {
		/* JIS code classification */
		unsigned char lb = lead;
		unsigned char tb = trail;

		/* convert process code to JIS */
# if defined(WIN3264) || defined(WIN32UNIX) || defined(MACOS)
		/* process code is SJIS */
		if (lb <= 0x9f)
		    lb = (lb - 0x81) * 2 + 0x21;
		else
		    lb = (lb - 0xc1) * 2 + 0x21;
		if (tb <= 0x7e)
		    tb -= 0x1f;
		else if (tb <= 0x9e)
		    tb -= 0x20;
		else
		{
		    tb -= 0x7e;
		    lb += 1;
		}
# else
		/*
		 * XXX: Code page identification can not use with all
		 *	    system! So, some other encoding information
		 *	    will be needed.
		 *	    In japanese: SJIS,EUC,UNICODE,(JIS)
		 *	    Note that JIS-code system don't use as
		 *	    process code in most system because it uses
		 *	    escape sequences(JIS is context depend encoding).
		 */
		/* assume process code is JAPANESE-EUC */
		lb &= 0x7f;
		tb &= 0x7f;
# endif
		/* exceptions */
		switch (lb << 8 | tb)
		{
		    case 0x213c: /* prolongedsound handled as KATAKANA */
			return 13;
		}
		/* sieved by KU code */
		switch (lb)
		{
		    case 0x21:
		    case 0x22:
			/* special symbols */
			return 10;
		    case 0x23:
			/* alpha-numeric */
			return 11;
		    case 0x24:
			/* hiragana */
			return 12;
		    case 0x25:
			/* katakana */
			return 13;
		    case 0x26:
			/* greek */
			return 14;
		    case 0x27:
			/* russian */
			return 15;
		    case 0x28:
			/* lines */
			return 16;
		    default:
			/* kanji */
			return 17;
		}
	    }

	case DBCS_KORU:	/* ? */
	case DBCS_KOR:
	    {
		/* KS code classification */
		unsigned char c1 = lead;
		unsigned char c2 = trail;

		/*
		 * 20 : Hangul
		 * 21 : Hanja
		 * 22 : Symbols
		 * 23 : Alpha-numeric/Roman Letter (Full width)
		 * 24 : Hangul Letter(Alphabet)
		 * 25 : Roman Numeral/Greek Letter
		 * 26 : Box Drawings
		 * 27 : Unit Symbols
		 * 28 : Circled/Parenthesized Letter
		 * 29 : Hirigana/Katakana
		 * 30 : Cyrillic Letter
		 */

		if (c1 >= 0xB0 && c1 <= 0xC8)
		    /* Hangul */
		    return 20;
#if defined(WIN3264) || defined(WIN32UNIX)
		else if (c1 <= 0xA0 || c2 <= 0xA0)
		    /* Extended Hangul Region : MS UHC(Unified Hangul Code) */
		    /* c1: 0x81-0xA0 with c2: 0x41-0x5A, 0x61-0x7A, 0x81-0xFE
		     * c1: 0xA1-0xC6 with c2: 0x41-0x5A, 0x61-0x7A, 0x81-0xA0
		     */
		    return 20;
#endif

		else if (c1 >= 0xCA && c1 <= 0xFD)
		    /* Hanja */
		    return 21;
		else switch (c1)
		{
		    case 0xA1:
		    case 0xA2:
			/* Symbols */
			return 22;
		    case 0xA3:
			/* Alpha-numeric */
			return 23;
		    case 0xA4:
			/* Hangul Letter(Alphabet) */
			return 24;
		    case 0xA5:
			/* Roman Numeral/Greek Letter */
			return 25;
		    case 0xA6:
			/* Box Drawings */
			return 26;
		    case 0xA7:
			/* Unit Symbols */
			return 27;
		    case 0xA8:
		    case 0xA9:
			if (c2 <= 0xAF)
			    return 25;  /* Roman Letter */
			else if (c2 >= 0xF6)
			    return 22;  /* Symbols */
			else
			    /* Circled/Parenthesized Letter */
			    return 28;
		    case 0xAA:
		    case 0xAB:
			/* Hirigana/Katakana */
			return 29;
		    case 0xAC:
			/* Cyrillic Letter */
			return 30;
		}
	    }
	default:
	    break;
    }
    return 3;
}

/*
 * mb_char2len() function pointer.
 * Return length in bytes of character "c".
 * Returns 1 for a single-byte character.
 */
/* ARGSUSED */
    int
latin_char2len(c)
    int		c;
{
    return 1;
}

    static int
dbcs_char2len(c)
    int		c;
{
    if (c >= 0x100)
	return 2;
    return 1;
}

/*
 * mb_char2bytes() function pointer.
 * Convert a character to its bytes.
 * Returns the length in bytes.
 */
    int
latin_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    buf[0] = c;
    return 1;
}

    static int
dbcs_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    if (c >= 0x100)
    {
	buf[0] = (unsigned)c >> 8;
	buf[1] = c;
	return 2;
    }
    buf[0] = c;
    return 1;
}

/*
 * mb_ptr2len_check() function pointer.
 * Get byte length of character at "*p" but stop at a NUL.
 * For UTF-8 this includes following composing characters.
 * Returns 0 when *p is NUL.
 *
 */
    int
latin_ptr2len_check(p)
    char_u	*p;
{
    return MB_BYTE2LEN(*p);
}

    static int
dbcs_ptr2len_check(p)
    char_u	*p;
{
    int		len;

    /* Check if second byte is not missing. */
    len = MB_BYTE2LEN(*p);
    if (len == 2 && p[1] == NUL)
	len = 1;
    return len;
}

/*
 * For UTF-8 character "c" return 2 for a double-width character, 1 for others.
 */
    int
utf_char2cells(c)
    int		c;
{
    if (c <= 0x9f && c >= 0x80)	    /* unprintable, displays <xx> */
	return 4;
    if (c >= 0x100 && !utf_printable(c))
	return 6;		    /* unprintable, displays <xxxx> */
    if (c >= 0x1100
	    && (c <= 0x115f			/* Hangul Jamo */
		|| (c >= 0x2e80 && c <= 0xa4cf && (c & ~0x0011) != 0x300a
		    && c != 0x303f)		/* CJK ... Yi */
		|| (c >= 0xac00 && c <= 0xd7a3)	/* Hangul Syllables */
		|| (c >= 0xf900 && c <= 0xfaff)	/* CJK Compatibility
						   Ideographs */
		|| (c >= 0xfe30 && c <= 0xfe6f)	/* CJK Compatibility Forms */
		|| (c >= 0xff00 && c <= 0xff5f)	/* Fullwidth Forms */
		|| (c >= 0xffe0 && c <= 0xffe6)
		|| (c >= 0x20000 && c <= 0x2ffff)))
	return 2;
    return 1;
}

/*
 * mb_ptr2cells() function pointer.
 * Return the number of display cells character at "*p" occupies.
 * This doesn't take care of unprintable characters, use ptr2cells() for that.
 */
/*ARGSUSED*/
    int
latin_ptr2cells(p)
    char_u	*p;
{
    return 1;
}

    int
utf_ptr2cells(p)
    char_u	*p;
{
    int		c;

    /* Need to convert to a wide character. */
    if (*p >= 0x80)
    {
	c = utf_ptr2char(p);
	/* An illegal byte is displayed as <xx>. */
	if (utf_ptr2len_check(p) == 1 || c == NUL)
	    return 4;
	/* If the char is ASCII it must be an overlong sequence. */
	if (c < 0x80)
	    return char2cells(c);
	return utf_char2cells(c);
    }
    return 1;
}

    int
dbcs_ptr2cells(p)
    char_u	*p;
{
    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && *p == 0x8e)
	return 1;
    return MB_BYTE2LEN(*p);
}

/*
 * mb_char2cells() function pointer.
 * Return the number of display cells character "c" occupies.
 * Only takes care of multi-byte chars, not "^C" and such.
 */
/*ARGSUSED*/
    int
latin_char2cells(c)
    int		c;
{
    return 1;
}

    static int
dbcs_char2cells(c)
    int		c;
{
    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && ((unsigned)c >> 8) == 0x8e)
	return 1;
    /* use the first byte */
    return MB_BYTE2LEN((unsigned)c >> 8);
}

/*
 * mb_off2cells() function pointer.
 * Return number of display cells for char at ScreenLines[off].
 * Caller must make sure "off" and "off + 1" are valid!
 */
/*ARGSUSED*/
    int
latin_off2cells(off)
    unsigned	off;
{
    return 1;
}

    int
dbcs_off2cells(off)
    unsigned	off;
{
    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && ScreenLines[off] == 0x8e)
	return 1;
    return MB_BYTE2LEN(ScreenLines[off]);
}

    int
utf_off2cells(off)
    unsigned	off;
{
    return ScreenLines[off + 1] == 0 ? 2 : 1;
}

/*
 * mb_ptr2char() function pointer.
 * Convert a byte sequence into a character.
 */
    int
latin_ptr2char(p)
    char_u	*p;
{
    return *p;
}

    static int
dbcs_ptr2char(p)
    char_u	*p;
{
    if (MB_BYTE2LEN(*p) > 1 && p[1] != NUL)
	return (p[0] << 8) + p[1];
    return *p;
}

/*
 * Convert a UTF-8 byte sequence to a wide character.
 * If the sequence is illegal or truncated by a NUL the first byte is
 * returned.
 * Does not include composing characters, of course.
 */
    int
utf_ptr2char(p)
    char_u	*p;
{
    int		len;

    if (p[0] < 0x80)	/* be quick for ASCII */
	return p[0];

    len = utf8len_tab[p[0]];
    if ((p[1] & 0xc0) == 0x80)
    {
	if (len == 2)
	    return ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
	if ((p[2] & 0xc0) == 0x80)
	{
	    if (len == 3)
		return ((p[0] & 0x0f) << 12) + ((p[1] & 0x3f) << 6)
		    + (p[2] & 0x3f);
	    if ((p[3] & 0xc0) == 0x80)
	    {
		if (len == 4)
		    return ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12)
			+ ((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
		if ((p[4] & 0xc0) == 0x80)
		{
		    if (len == 5)
			return ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18)
			    + ((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6)
			    + (p[4] & 0x3f);
		    if ((p[5] & 0xc0) == 0x80 && len == 6)
			return ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24)
			    + ((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12)
			    + ((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
		}
	    }
	}
    }
    /* Illegal value, just return the first byte */
    return p[0];
}

/*
 * Get character at **pp and advance *pp to the next character.
 * Note: composing characters are skipped!
 */
    int
mb_ptr2char_adv(pp)
    char_u	**pp;
{
    int		c;

    c = (*mb_ptr2char)(*pp);
    *pp += (*mb_ptr2len_check)(*pp);
    return c;
}

/*
 * Convert a UTF-8 byte string to a wide chararacter.  Also get up to two
 * composing characters.
 */
    int
utfc_ptr2char(p, p1, p2)
    char_u	*p;
    int		*p1;	/* return: first composing char or 0 */
    int		*p2;	/* return: second composing char or 0 */
{
    int		len;
    int		c;
    int		cc;

    c = utf_ptr2char(p);
    len = utf_ptr2len_check(p);
    /* Only accept a composing char when the first char isn't illegal. */
    if ((len > 1 || *p < 0x80)
	    && p[len] >= 0x80 && utf_iscomposing(cc = utf_ptr2char(p + len)))
    {
	*p1 = cc;
	len += utf_ptr2len_check(p + len);
	if (p[len] >= 0x80 && utf_iscomposing(cc = utf_ptr2char(p + len)))
	    *p2 = cc;
	else
	    *p2 = 0;
    }
    else
    {
	*p1 = 0;
	*p2 = 0;
    }
    return c;
}

/*
 * Convert the character at screen position "off" to a sequence of bytes.
 * Includes the composing characters.
 * "buf" must at least have the length MB_MAXBYTES.
 * Returns the produced number of bytes.
 */
    int
utfc_char2bytes(off, buf)
    int		off;
    char_u	*buf;
{
    int		len;

    len = utf_char2bytes(ScreenLinesUC[off], buf);
    if (ScreenLinesC1[off] != 0)
    {
	len += utf_char2bytes(ScreenLinesC1[off], buf + len);
	if (ScreenLinesC2[off] != 0)
	    len += utf_char2bytes(ScreenLinesC2[off], buf + len);
    }
    return len;
}

/*
 * Get the length of a UTF-8 byte sequence.  Ignores any following composing
 * characters.
 * Returns 0 for "".
 * Returns 1 for an illegal byte sequence.
 */
    int
utf_ptr2len_check(p)
    char_u	*p;
{
    int		len;
    int		i;

    if (*p == NUL)
	return 0;
    len = utf8len_tab[*p];
    for (i = 1; i < len; ++i)
	if ((p[i] & 0xc0) != 0x80)
	    return 1;
    return len;
}

/*
 * Return length of UTF-8 character, obtained from the first byte.
 * "b" must be between 0 and 255!
 */
    int
utf_byte2len(b)
    int		b;
{
    return utf8len_tab[b];
}

/*
 * Get the length of UTF-8 byte sequence "p[size]".  Ignores any following
 * composing characters.
 * Returns 1 for "".
 * Returns 1 for an illegal byte sequence.
 * Returns number > "size" for an incomplete byte sequence.
 */
    int
utf_ptr2len_check_len(p, size)
    char_u	*p;
    int		size;
{
    int		len;
    int		i;

    if (*p == NUL)
	return 1;
    len = utf8len_tab[*p];
    for (i = 1; i < len && i < size; ++i)
	if ((p[i] & 0xc0) != 0x80)
	    return 1;
    return len;
}

/*
 * Return the number of bytes the UTF-8 encoding of the character at "p" takes.
 * This includes following composing characters.
 */
    int
utfc_ptr2len_check(p)
    char_u	*p;
{
    int		len;

    if (*p == NUL)
	return 0;
    if (p[0] < 0x80 && p[1] < 0x80)	/* be quick for ASCII */
	return 1;

    /* Skip over first UTF-8 char, stopping at a NUL byte. */
    len = utf_ptr2len_check(p);

    /* Check for illegal byte. */
    if (len == 1 && p[0] >= 0x80)
	return 1;

    /*
     * Check for composing characters.  We can handle only the first two, but
     * skip all of them (otherwise the cursor would get stuck).
     */
    for (;;)
    {
	if (p[len] < 0x80 || !utf_iscomposing(utf_ptr2char(p + len)))
	    return len;

	/* Skip over composing char */
	len += utf_ptr2len_check(p + len);
    }
}

/*
 * Return the number of bytes the UTF-8 encoding of character "c" takes.
 * This does not include composing characters.
 */
    int
utf_char2len(c)
    int		c;
{
    if (c < 0x80)
	return 1;
    if (c < 0x800)
	return 2;
    if (c < 0x10000)
	return 3;
    if (c < 0x200000)
	return 4;
    if (c < 0x4000000)
	return 5;
    return 6;
}

/*
 * Convert UTF-8 character "c" to string of bytes in "buf[]".
 * Returns the number of bytes.
 * This does not include composing characters.
 */
    int
utf_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    if (c < 0x80)		/* 7 bits */
    {
	buf[0] = c;
	return 1;
    }
    if (c < 0x800)		/* 11 bits */
    {
	buf[0] = 0xc0 + ((unsigned)c >> 6);
	buf[1] = 0x80 + (c & 0x3f);
	return 2;
    }
    if (c < 0x10000)		/* 16 bits */
    {
	buf[0] = 0xe0 + ((unsigned)c >> 12);
	buf[1] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[2] = 0x80 + (c & 0x3f);
	return 3;
    }
    if (c < 0x200000)		/* 21 bits */
    {
	buf[0] = 0xf0 + ((unsigned)c >> 18);
	buf[1] = 0x80 + (((unsigned)c >> 12) & 0x3f);
	buf[2] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[3] = 0x80 + (c & 0x3f);
	return 4;
    }
    if (c < 0x4000000)		/* 26 bits */
    {
	buf[0] = 0xf8 + ((unsigned)c >> 24);
	buf[1] = 0x80 + (((unsigned)c >> 18) & 0x3f);
	buf[2] = 0x80 + (((unsigned)c >> 12) & 0x3f);
	buf[3] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[4] = 0x80 + (c & 0x3f);
	return 5;
    }
				/* 31 bits */
    buf[0] = 0xfc + ((unsigned)c >> 30);
    buf[1] = 0x80 + (((unsigned)c >> 24) & 0x3f);
    buf[2] = 0x80 + (((unsigned)c >> 18) & 0x3f);
    buf[3] = 0x80 + (((unsigned)c >> 12) & 0x3f);
    buf[4] = 0x80 + (((unsigned)c >> 6) & 0x3f);
    buf[5] = 0x80 + (c & 0x3f);
    return 6;
}

struct interval
{
    unsigned short first;
    unsigned short last;
};
static int intable __ARGS((struct interval *table, int size, int c));

/*
 * Return TRUE if "c" is in "table[size]".
 */
    static int
intable(table, size, c)
    struct interval	*table;
    int			size;
    int			c;
{
    int mid, bot, top;

    /* first quick check for Latin1 etc. characters */
    if (c < table[0].first)
	return FALSE;

    /* binary search in table */
    bot = 0;
    top = size - 1;
    while (top >= bot)
    {
	mid = (bot + top) / 2;
	if (table[mid].last < c)
	    bot = mid + 1;
	else if (table[mid].first > c)
	    top = mid - 1;
	else
	    return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if "c" is a composing UTF-8 character.  This means it will be
 * drawn on top of the preceding character.
 * Based on code from Markus Kuhn.
 */
    int
utf_iscomposing(c)
    int		c;
{
    /* sorted list of non-overlapping intervals */
    static struct interval combining[] =
    {
	{0x0300, 0x034E}, {0x0360, 0x0362}, {0x0483, 0x0486}, {0x0488, 0x0489},
	{0x0591, 0x05A1}, {0x05A3, 0x05B9}, {0x05BB, 0x05BD}, {0x05BF, 0x05BF},
	{0x05C1, 0x05C2}, {0x05C4, 0x05C4}, {0x064B, 0x0655}, {0x0670, 0x0670},
	{0x06D6, 0x06E4}, {0x06E7, 0x06E8}, {0x06EA, 0x06ED}, {0x0711, 0x0711},
	{0x0730, 0x074A}, {0x07A6, 0x07B0}, {0x0901, 0x0902}, {0x093C, 0x093C},
	{0x0941, 0x0948}, {0x094D, 0x094D}, {0x0951, 0x0954}, {0x0962, 0x0963},
	{0x0981, 0x0981}, {0x09BC, 0x09BC}, {0x09C1, 0x09C4}, {0x09CD, 0x09CD},
	{0x09E2, 0x09E3}, {0x0A02, 0x0A02}, {0x0A3C, 0x0A3C}, {0x0A41, 0x0A42},
	{0x0A47, 0x0A48}, {0x0A4B, 0x0A4D}, {0x0A70, 0x0A71}, {0x0A81, 0x0A82},
	{0x0ABC, 0x0ABC}, {0x0AC1, 0x0AC5}, {0x0AC7, 0x0AC8}, {0x0ACD, 0x0ACD},
	{0x0B01, 0x0B01}, {0x0B3C, 0x0B3C}, {0x0B3F, 0x0B3F}, {0x0B41, 0x0B43},
	{0x0B4D, 0x0B4D}, {0x0B56, 0x0B56}, {0x0B82, 0x0B82}, {0x0BC0, 0x0BC0},
	{0x0BCD, 0x0BCD}, {0x0C3E, 0x0C40}, {0x0C46, 0x0C48}, {0x0C4A, 0x0C4D},
	{0x0C55, 0x0C56}, {0x0CBF, 0x0CBF}, {0x0CC6, 0x0CC6}, {0x0CCC, 0x0CCD},
	{0x0D41, 0x0D43}, {0x0D4D, 0x0D4D}, {0x0DCA, 0x0DCA}, {0x0DD2, 0x0DD4},
	{0x0DD6, 0x0DD6}, {0x0E31, 0x0E31}, {0x0E34, 0x0E3A}, {0x0E47, 0x0E4E},
	{0x0EB1, 0x0EB1}, {0x0EB4, 0x0EB9}, {0x0EBB, 0x0EBC}, {0x0EC8, 0x0ECD},
	{0x0F18, 0x0F19}, {0x0F35, 0x0F35}, {0x0F37, 0x0F37}, {0x0F39, 0x0F39},
	{0x0F71, 0x0F7E}, {0x0F80, 0x0F84}, {0x0F86, 0x0F87}, {0x0F90, 0x0F97},
	{0x0F99, 0x0FBC}, {0x0FC6, 0x0FC6}, {0x102D, 0x1030}, {0x1032, 0x1032},
	{0x1036, 0x1037}, {0x1039, 0x1039}, {0x1058, 0x1059}, {0x17B7, 0x17BD},
	{0x17C6, 0x17C6}, {0x17C9, 0x17D3}, {0x18A9, 0x18A9}, {0x20D0, 0x20E3},
	{0x302A, 0x302F}, {0x3099, 0x309A}, {0xFB1E, 0xFB1E}, {0xFE20, 0xFE23}
    };

    return intable(combining, sizeof(combining) / sizeof(struct interval), c);
}

/*
 * Return TRUE for characters that can be displayed in a normal way.
 * Only for characters of 0x100 and above!
 */
    int
utf_printable(c)
    int		c;
{
    /* sorted list of non-overlapping intervals */
    static struct interval nonprint[] =
    {
	{0x070f, 0x070f}, {0x180b, 0x180e}, {0x200b, 0x200f}, {0x202a, 0x202e},
	{0x206a, 0x206f}, {0xfeff, 0xfeff}, {0xfff9, 0xfffb}
    };

    return !intable(nonprint, sizeof(nonprint) / sizeof(struct interval), c);
}

/*
 * Get class of a Unicode character.
 * 0: white space
 * 1: punctuation
 * 2 or bigger: some class of word character.
 */
    int
utf_class(c)
    int		c;
{
    /* sorted list of non-overlapping intervals */
    static struct interval
    {
	unsigned short first;
	unsigned short last;
	unsigned short class;
    } classes[] =
    {
	{0x037e, 0x037e, 1},		/* Greek question mark */
	{0x0387, 0x0387, 1},		/* Greek ano teleia */
	{0x055a, 0x055f, 1},		/* Armenian punctuation */
	{0x0589, 0x0589, 1},		/* Armenian full stop */
	{0x0700, 0x070d, 1},		/* Syriac punctuation */
	{0x104a, 0x104f, 1},		/* Myanmar punctuation */
	{0x10fb, 0x10fb, 1},		/* Georgian punctuation */
	{0x1361, 0x1368, 1},		/* Ethiopic punctuation */
	{0x166d, 0x166e, 1},		/* Canadian Syl. punctuation */
	{0x17d4, 0x17dc, 1},		/* Khmer punctuation */
	{0x1800, 0x180a, 1},		/* Mongolian punctuation */
	{0x2000, 0x200a, 0},		/* spaces */
	{0x200b, 0x27ff, 1},		/* punctuation and symbols */
	{0x2070, 0x207f, 0x2070},	/* superscript */
	{0x2080, 0x208f, 0x2080},	/* subscript */
	{0x3000, 0x3000, 0},		/* ideographic space */
	{0x3001, 0x3020, 1},		/* ideographic punctuation */
	{0x3040, 0x309f, 0x3040},	/* Hiragana */
	{0x30a0, 0x30ff, 0x30a0},	/* Katakana */
	{0x3300, 0x9fff, 0x4e00},	/* CJK Ideographs */
	{0xac00, 0xd7a3, 0xac00},	/* Hangul Syllables */
	{0xf900, 0xfaff, 0x4e00},	/* CJK Ideographs */
	{0xfe30, 0xfe6b, 1},		/* punctuation forms */
	{0xff00, 0xff0f, 1},		/* half/fullwidth ASCII */
	{0xff1a, 0xff20, 1},		/* half/fullwidth ASCII */
	{0xff3b, 0xff40, 1},		/* half/fullwidth ASCII */
	{0xff5b, 0xff64, 1},		/* half/fullwidth ASCII */
    };
    int bot = 0;
    int top = sizeof(classes) / sizeof(struct interval) - 1;
    int mid;

    /* First quick check for Latin1 characters, use 'iskeyword'. */
    if (c < 0x100)
    {
	if (c == ' ' || c == '\t' || c == NUL)
	    return 0;	    /* blank */
	if (vim_iswordc(c))
	    return 2;	    /* word character */
	return 1;	    /* punctuation */
    }

    /* binary search in table */
    while (top >= bot)
    {
	mid = (bot + top) / 2;
	if (classes[mid].last < c)
	    bot = mid + 1;
	else if (classes[mid].first > c)
	    top = mid - 1;
	else
	    return (int)classes[mid].class;
    }

    /* most other characters are "word" characters */
    return 2;
}

/*
 * Code for Unicode case-dependent operations.  Based on notes in
 * http://www.unicode.org/Public/UNIDATA/CaseFolding.txt
 * This code uses simple case folding, not full case folding.
 */

/*
 * The following table is built by foldExtract.pl < CaseFolding.txt .
 * It must be in numeric order, because we use binary search on it.
 * An entry such as {0x41,0x5a,1,32} means that UCS-4 characters in the range
 * from 0x41 to 0x5a inclusive, stepping by 1, are folded by adding 32.
 */

typedef struct
{
    int rangeStart;
    int rangeEnd;
    int step;
    int offset;
} convertStruct;

convertStruct foldCase[] =
{
	{0x41,0x5a,1,32}, {0xb5,0xb5,-1,775}, {0xc0,0xd6,1,32},
	{0xd8,0xde,1,32}, {0x100,0x12e,2,1}, {0x130,0x130,-1,-199},
	{0x131,0x131,-1,-200}, {0x132,0x136,2,1}, {0x139,0x147,2,1},
	{0x14a,0x176,2,1}, {0x178,0x178,-1,-121}, {0x179,0x17d,2,1},
	{0x17f,0x17f,-1,-268}, {0x181,0x181,-1,210}, {0x182,0x184,2,1},
	{0x186,0x186,-1,206}, {0x187,0x187,-1,1}, {0x189,0x18a,1,205},
	{0x18b,0x18b,-1,1}, {0x18e,0x18e,-1,79}, {0x18f,0x18f,-1,202},
	{0x190,0x190,-1,203}, {0x191,0x191,-1,1}, {0x193,0x193,-1,205},
	{0x194,0x194,-1,207}, {0x196,0x196,-1,211}, {0x197,0x197,-1,209},
	{0x198,0x198,-1,1}, {0x19c,0x19c,-1,211}, {0x19d,0x19d,-1,213},
	{0x19f,0x19f,-1,214}, {0x1a0,0x1a4,2,1}, {0x1a6,0x1a6,-1,218},
	{0x1a7,0x1a7,-1,1}, {0x1a9,0x1a9,-1,218}, {0x1ac,0x1ac,-1,1},
	{0x1ae,0x1ae,-1,218}, {0x1af,0x1af,-1,1}, {0x1b1,0x1b2,1,217},
	{0x1b3,0x1b5,2,1}, {0x1b7,0x1b7,-1,219}, {0x1b8,0x1bc,4,1},
	{0x1c4,0x1c4,-1,2}, {0x1c5,0x1c5,-1,1}, {0x1c7,0x1c7,-1,2},
	{0x1c8,0x1c8,-1,1}, {0x1ca,0x1ca,-1,2}, {0x1cb,0x1db,2,1},
	{0x1de,0x1ee,2,1}, {0x1f1,0x1f1,-1,2}, {0x1f2,0x1f4,2,1},
	{0x1f6,0x1f6,-1,-97}, {0x1f7,0x1f7,-1,-56}, {0x1f8,0x21e,2,1},
	{0x222,0x232,2,1}, {0x345,0x345,-1,116}, {0x386,0x386,-1,38},
	{0x388,0x38a,1,37}, {0x38c,0x38c,-1,64}, {0x38e,0x38f,1,63},
	{0x391,0x3a1,1,32}, {0x3a3,0x3ab,1,32}, {0x3c2,0x3c2,-1,1},
	{0x3d0,0x3d0,-1,-30}, {0x3d1,0x3d1,-1,-25}, {0x3d5,0x3d5,-1,-15},
	{0x3d6,0x3d6,-1,-22}, {0x3da,0x3ee,2,1}, {0x3f0,0x3f0,-1,-54},
	{0x3f1,0x3f1,-1,-48}, {0x3f2,0x3f2,-1,-47}, {0x3f4,0x3f4,-1,-60},
	{0x3f5,0x3f5,-1,-64}, {0x400,0x40f,1,80}, {0x410,0x42f,1,32},
	{0x460,0x480,2,1}, {0x48c,0x4be,2,1}, {0x4c1,0x4c3,2,1},
	{0x4c7,0x4cb,4,1}, {0x4d0,0x4f4,2,1}, {0x4f8,0x4f8,-1,1},
	{0x531,0x556,1,48}, {0x1e00,0x1e94,2,1}, {0x1e9b,0x1e9b,-1,-58},
	{0x1ea0,0x1ef8,2,1}, {0x1f08,0x1f0f,1,-8}, {0x1f18,0x1f1d,1,-8},
	{0x1f28,0x1f2f,1,-8}, {0x1f38,0x1f3f,1,-8}, {0x1f48,0x1f4d,1,-8},
	{0x1f59,0x1f5f,2,-8}, {0x1f68,0x1f6f,1,-8}, {0x1f88,0x1f8f,1,-8},
	{0x1f98,0x1f9f,1,-8}, {0x1fa8,0x1faf,1,-8}, {0x1fb8,0x1fb9,1,-8},
	{0x1fba,0x1fbb,1,-74}, {0x1fbc,0x1fbc,-1,-9}, {0x1fbe,0x1fbe,-1,-7173},
	{0x1fc8,0x1fcb,1,-86}, {0x1fcc,0x1fcc,-1,-9}, {0x1fd8,0x1fd9,1,-8},
	{0x1fda,0x1fdb,1,-100}, {0x1fe8,0x1fe9,1,-8}, {0x1fea,0x1feb,1,-112},
	{0x1fec,0x1fec,-1,-7}, {0x1ff8,0x1ff9,1,-128}, {0x1ffa,0x1ffb,1,-126},
	{0x1ffc,0x1ffc,-1,-9}, {0x2126,0x2126,-1,-7517}, {0x212a,0x212a,-1,-8383},
	{0x212b,0x212b,-1,-8262}, {0x2160,0x216f,1,16}, {0x24b6,0x24cf,1,26},
	{0xff21,0xff3a,1,32}, {0x10400,0x10425,1,40}
	};

static int utf_convert(int a, convertStruct table[], int tableSize);

/*
 * Generic conversion function for case operations.
 * Return the converted equivalent of "a", which is a UCS-4 character.  Use
 * the given conversion "table".  Uses binary search on "table".
 */
    static int
utf_convert(a, table, tableSize)
    int			a;
    convertStruct	table[];
    int			tableSize;
{
    int start, mid, end; /* indices into table */

    start = 0;
    end = tableSize / sizeof(convertStruct);
    while (start < end)
    {
	/* need to search further */
	mid = (end + start) /2;
	if (table[mid].rangeEnd < a)
	    start = mid + 1;
	else
	    end = mid;
    }
    if (table[start].rangeStart <= a && a <= table[start].rangeEnd
	    && (a - table[start].rangeStart) % table[start].step == 0)
	return (a + table[start].offset);
    else
	return a;
}

/*
 * Return the folded-case equivalent of "a", which is a UCS-4 character.  Uses
 * simple case folding.
 */
    int
utf_fold(a)
    int		a;
{
    return utf_convert(a, foldCase, sizeof(foldCase));
}

/*
 * The following tables are built by upperLowerExtract.pl < UnicodeData.txt .
 * They must be in numeric order, because we use binary search on them.
 * An entry such as {0x41,0x5a,1,32} means that UCS-4 characters in the range
 * from 0x41 to 0x5a inclusive, stepping by 1, are switched to lower (for
 * example) by adding 32.
 */
convertStruct toLower[] =
{
	{0x41,0x5a,1,32}, {0xc0,0xd6,1,32}, {0xd8,0xde,1,32},
	{0x100,0x12e,2,1}, {0x130,0x130,-1,-199}, {0x132,0x136,2,1},
	{0x139,0x147,2,1}, {0x14a,0x176,2,1}, {0x178,0x178,-1,-121},
	{0x179,0x17d,2,1}, {0x181,0x181,-1,210}, {0x182,0x184,2,1},
	{0x186,0x186,-1,206}, {0x187,0x187,-1,1}, {0x189,0x18a,1,205},
	{0x18b,0x18b,-1,1}, {0x18e,0x18e,-1,79}, {0x18f,0x18f,-1,202},
	{0x190,0x190,-1,203}, {0x191,0x191,-1,1}, {0x193,0x193,-1,205},
	{0x194,0x194,-1,207}, {0x196,0x196,-1,211}, {0x197,0x197,-1,209},
	{0x198,0x198,-1,1}, {0x19c,0x19c,-1,211}, {0x19d,0x19d,-1,213},
	{0x19f,0x19f,-1,214}, {0x1a0,0x1a4,2,1}, {0x1a6,0x1a6,-1,218},
	{0x1a7,0x1a7,-1,1}, {0x1a9,0x1a9,-1,218}, {0x1ac,0x1ac,-1,1},
	{0x1ae,0x1ae,-1,218}, {0x1af,0x1af,-1,1}, {0x1b1,0x1b2,1,217},
	{0x1b3,0x1b5,2,1}, {0x1b7,0x1b7,-1,219}, {0x1b8,0x1bc,4,1},
	{0x1c4,0x1ca,3,2}, {0x1cd,0x1db,2,1}, {0x1de,0x1ee,2,1},
	{0x1f1,0x1f1,-1,2}, {0x1f4,0x1f4,-1,1}, {0x1f6,0x1f6,-1,-97},
	{0x1f7,0x1f7,-1,-56}, {0x1f8,0x21e,2,1}, {0x222,0x232,2,1},
	{0x386,0x386,-1,38}, {0x388,0x38a,1,37}, {0x38c,0x38c,-1,64},
	{0x38e,0x38f,1,63}, {0x391,0x3a1,1,32}, {0x3a3,0x3ab,1,32},
	{0x3da,0x3ee,2,1}, {0x3f4,0x3f4,-1,-60}, {0x400,0x40f,1,80},
	{0x410,0x42f,1,32}, {0x460,0x480,2,1}, {0x48c,0x4be,2,1},
	{0x4c1,0x4c3,2,1}, {0x4c7,0x4cb,4,1}, {0x4d0,0x4f4,2,1},
	{0x4f8,0x4f8,-1,1}, {0x531,0x556,1,48}, {0x1e00,0x1e94,2,1},
	{0x1ea0,0x1ef8,2,1}, {0x1f08,0x1f0f,1,-8}, {0x1f18,0x1f1d,1,-8},
	{0x1f28,0x1f2f,1,-8}, {0x1f38,0x1f3f,1,-8}, {0x1f48,0x1f4d,1,-8},
	{0x1f59,0x1f5f,2,-8}, {0x1f68,0x1f6f,1,-8}, {0x1fb8,0x1fb9,1,-8},
	{0x1fba,0x1fbb,1,-74}, {0x1fc8,0x1fcb,1,-86}, {0x1fd8,0x1fd9,1,-8},
	{0x1fda,0x1fdb,1,-100}, {0x1fe8,0x1fe9,1,-8}, {0x1fea,0x1feb,1,-112},
	{0x1fec,0x1fec,-1,-7}, {0x1ff8,0x1ff9,1,-128}, {0x1ffa,0x1ffb,1,-126},
	{0x2126,0x2126,-1,-7517}, {0x212a,0x212a,-1,-8383},
	{0x212b,0x212b,-1,-8262}, {0xff21,0xff3a,1,32}, {0x10400,0x10425,1,40}
};

convertStruct toUpper[] =
{
	{0x61,0x7a,1,-32}, {0xb5,0xb5,-1,743}, {0xe0,0xf6,1,-32},
	{0xf8,0xfe,1,-32}, {0xff,0xff,-1,121}, {0x101,0x12f,2,-1},
	{0x131,0x131,-1,-232}, {0x133,0x137,2,-1}, {0x13a,0x148,2,-1},
	{0x14b,0x177,2,-1}, {0x17a,0x17e,2,-1}, {0x17f,0x17f,-1,-300},
	{0x183,0x185,2,-1}, {0x188,0x18c,4,-1}, {0x192,0x192,-1,-1},
	{0x195,0x195,-1,97}, {0x199,0x1a1,8,-1}, {0x1a3,0x1a5,2,-1},
	{0x1a8,0x1ad,5,-1}, {0x1b0,0x1b4,4,-1}, {0x1b6,0x1b9,3,-1},
	{0x1bd,0x1bd,-1,-1}, {0x1bf,0x1bf,-1,56}, {0x1c6,0x1cc,3,-1},
	{0x1ce,0x1dc,2,-1}, {0x1dd,0x1dd,-1,-79}, {0x1df,0x1ef,2,-1},
	{0x1f3,0x1f5,2,-1}, {0x1f9,0x21f,2,-1}, {0x223,0x233,2,-1},
	{0x253,0x253,-1,-210}, {0x254,0x254,-1,-206}, {0x256,0x257,1,-205},
	{0x259,0x259,-1,-202}, {0x25b,0x25b,-1,-203}, {0x260,0x260,-1,-205},
	{0x263,0x263,-1,-207}, {0x268,0x268,-1,-209}, {0x269,0x26f,6,-211},
	{0x272,0x272,-1,-213}, {0x275,0x275,-1,-214}, {0x280,0x283,3,-218},
	{0x288,0x288,-1,-218}, {0x28a,0x28b,1,-217}, {0x292,0x292,-1,-219},
	{0x3ac,0x3ac,-1,-38}, {0x3ad,0x3af,1,-37}, {0x3b1,0x3c1,1,-32},
	{0x3c2,0x3c2,-1,-31}, {0x3c3,0x3cb,1,-32}, {0x3cc,0x3cc,-1,-64},
	{0x3cd,0x3ce,1,-63}, {0x3d0,0x3d0,-1,-62}, {0x3d1,0x3d1,-1,-57},
	{0x3d5,0x3d5,-1,-47}, {0x3d6,0x3d6,-1,-54}, {0x3db,0x3ef,2,-1},
	{0x3f0,0x3f0,-1,-86}, {0x3f1,0x3f1,-1,-80}, {0x3f2,0x3f2,-1,-79},
	{0x3f5,0x3f5,-1,-96}, {0x430,0x44f,1,-32}, {0x450,0x45f,1,-80},
	{0x461,0x481,2,-1}, {0x48d,0x4bf,2,-1}, {0x4c2,0x4c4,2,-1},
	{0x4c8,0x4cc,4,-1}, {0x4d1,0x4f5,2,-1}, {0x4f9,0x4f9,-1,-1},
	{0x561,0x586,1,-48}, {0x1e01,0x1e95,2,-1}, {0x1e9b,0x1e9b,-1,-59},
	{0x1ea1,0x1ef9,2,-1}, {0x1f00,0x1f07,1,8}, {0x1f10,0x1f15,1,8},
	{0x1f20,0x1f27,1,8}, {0x1f30,0x1f37,1,8}, {0x1f40,0x1f45,1,8},
	{0x1f51,0x1f57,2,8}, {0x1f60,0x1f67,1,8}, {0x1f70,0x1f71,1,74},
	{0x1f72,0x1f75,1,86}, {0x1f76,0x1f77,1,100}, {0x1f78,0x1f79,1,128},
	{0x1f7a,0x1f7b,1,112}, {0x1f7c,0x1f7d,1,126}, {0x1f80,0x1f87,1,8},
	{0x1f90,0x1f97,1,8}, {0x1fa0,0x1fa7,1,8}, {0x1fb0,0x1fb1,1,8},
	{0x1fb3,0x1fb3,-1,9}, {0x1fbe,0x1fbe,-1,-7205}, {0x1fc3,0x1fc3,-1,9},
	{0x1fd0,0x1fd1,1,8}, {0x1fe0,0x1fe1,1,8}, {0x1fe5,0x1fe5,-1,7},
	{0x1ff3,0x1ff3,-1,9}, {0xff41,0xff5a,1,-32}, {0x10428,0x1044d,1,-40}
};

/*
 * Return the upper-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
    int
utf_toupper(a)
    int		a;
{
    if (a < 128)
	return toupper(a);
    return utf_convert(a, toUpper, sizeof(toUpper));
}

    int
utf_islower(a)
    int		a;
{
    return (utf_toupper(a) != a);
}

/*
 * Return the lower-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
    int
utf_tolower(a)
    int		a;
{
    if (a < 128)
	return tolower(a);
    return utf_convert(a, toLower, sizeof(toLower));
}

    int
utf_isupper(a)
    int		a;
{
    return (utf_tolower(a) != a);
}

/*
 * Version of strnicmp() that handles multi-byte characters.
 * Only needed for Big5 and UTF-8 encoding.  Other DBCS encodings can use
 * strnicmp(), because there are no ASCII characters in the second byte.
 * Returns zero if s1 and s2 are equal (ignoring case), the difference between
 * two characters otherwise.
 */
    int
mb_strnicmp(s1, s2, n)
    char_u	*s1, *s2;
    int		n;
{
    int		i, l;
    int		cdiff;

    for (i = 0; i < n; i += l)
    {
	l = (*mb_ptr2len_check)(s1 + i);
	if (l <= 1)
	{
	    /* Single byte: first check normally, then with ignore case. */
	    if (s1[i] != s2[i])
	    {
		cdiff = TO_LOWER(s1[i]) - TO_LOWER(s2[i]);
		if (cdiff != 0)
		    return cdiff;
	    }
	    else if (s1[i] == NUL)
		return 0;
	}
	else
	{
	    /* For multi-byte only ignore case for Unicode. */
	    if (l > n - i)
		l = n - i;
	    if (enc_utf8)
		cdiff = utf_fold(utf_ptr2char(s1 + i))
					     - utf_fold(utf_ptr2char(s2 + i));
	    else
		cdiff =  STRNCMP(s1 + i, s2 + i, l);
	    if (cdiff != 0)
		return cdiff;
	}
    }
    return 0;
}

/*
 * "g8": show bytes of the UTF-8 char under the cursor.  Doesn't matter what
 * 'encoding' has been set to.
 */
    void
show_utf8()
{
    int		len;
    char_u	*line;
    int		clen;
    int		i;

    /* Get the byte length of the char under the cursor, including composing
     * characters. */
    line = ml_get_cursor();
    len = utfc_ptr2len_check(line);
    if (len == 0)
    {
	MSG("NUL");
	return;
    }

    IObuff[0] = NUL;
    clen = 0;
    for (i = 0; i < len; ++i)
    {
	if (clen == 0)
	{
	    /* start of (composing) character, get its length */
	    if (i > 0)
		STRCAT(IObuff, "+ ");
	    clen = utf_ptr2len_check(line + i);
	}
	sprintf((char *)IObuff + STRLEN(IObuff), "%02x ", line[i]);
	--clen;
    }

    msg(IObuff);
}

/*
 * mb_head_off() function pointer.
 * Return offset from "p" to the first byte of the character it points into.
 * Returns 0 when already at the first byte of a character.
 */
/*ARGSUSED*/
    int
latin_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    return 0;
}

    int
dbcs_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;

    /* It can't be a trailing byte when not using DBCS, at the start of the
     * string or the previous byte can't start a double-byte. */
    if (p <= base || MB_BYTE2LEN(p[-1]) == 1)
	return 0;

    /* This is slow: need to start at the base and go forward until the
     * byte we are looking for.  Return 1 when we went past it, 0 otherwise. */
    q = base;
    while (q < p)
	q += dbcs_ptr2len_check(q);
    return (q == p) ? 0 : 1;
}

#if defined(FEAT_CLIPBOARD) || defined(FEAT_GUI) || defined(PROTO)
/*
 * Special version of dbcs_head_off() that works for ScreenLines[], where
 * single-width DBCS_JPNU characters are stored separately.
 */
    int
dbcs_screen_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;

    /* It can't be a trailing byte when not using DBCS, at the start of the
     * string or the previous byte can't start a double-byte.
     * For euc-jp an 0x8e byte in the previous cell always means we have a
     * lead byte in the current cell. */
    if (p <= base
	    || (enc_dbcs == DBCS_JPNU && p[-1] == 0x8e)
	    || MB_BYTE2LEN(p[-1]) == 1)
	return 0;

    /* This is slow: need to start at the base and go forward until the
     * byte we are looking for.  Return 1 when we went past it, 0 otherwise.
     * For DBCS_JPNU look out for 0x8e, which means the second byte is not
     * stored as the next byte. */
    q = base;
    while (q < p)
    {
	if (enc_dbcs == DBCS_JPNU && *q == 0x8e)
	    ++q;
	else
	    q += dbcs_ptr2len_check(q);
    }
    return (q == p) ? 0 : 1;
}
#endif

    int
utf_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;
    char_u	*s;

    if (*p < 0x80)		/* be quick for ASCII */
	return 0;

    /* Skip backwards over trailing bytes: 10xx.xxxx
     * Skip backwards again if on a composing char. */
    for (q = p; ; --q)
    {
	/* Move s to the last byte of this char. */
	for (s = q; (s[1] & 0xc0) == 0x80; ++s)
	    ;
	/* Move q to the first byte of this char. */
	while (q > base && (*q & 0xc0) == 0x80)
	    --q;
	/* Check for illegal sequence. */
	if (utf8len_tab[*q] != (int)(s - q + 1))
	    return 0;
	if (q <= base || !utf_iscomposing(utf_ptr2char(q)))
	    break;
    }

    return (int)(p - q);
}

/*
 * Return the offset from "p" to the first byte of a character.  When "p" is
 * at the start of a character 0 is returned, otherwise the offset to the next
 * character.  Can start anywhere in a stream of bytes.
 */
    int
mb_off_next(base, p)
    char_u	*base;
    char_u	*p;
{
    int		i;
    int		j;

    if (enc_utf8)
    {
	if (*p < 0x80)		/* be quick for ASCII */
	    return 0;

	/* Find the next character that isn't 10xx.xxxx */
	for (i = 0; (p[i] & 0xc0) == 0x80; ++i)
	    ;
	if (i > 0)
	{
	    /* Check for illegal sequence. */
	    for (j = 0; p - j > base; ++j)
		if ((p[-j] & 0xc0) != 0x80)
		    break;
	    if (utf8len_tab[p[-j]] != i + j)
		return 0;
	}
	return i;
    }

    /* Only need to check if we're on a trail byte, it doesn't matter if we
     * want the offset to the next or current character. */
    return (*mb_head_off)(base, p);
}

/*
 * Return the offset from "p" to the last byte of the character it points
 * into.  Can start anywhere in a stream of bytes.
 */
    int
mb_tail_off(base, p)
    char_u	*base;
    char_u	*p;
{
    int		i;
    int		j;

    if (*p == NUL)
	return 0;

    if (enc_utf8)
    {
	/* Find the last character that is 10xx.xxxx */
	for (i = 0; (p[i + 1] & 0xc0) == 0x80; ++i)
	    ;
	/* Check for illegal sequence. */
	for (j = 0; p - j > base; ++j)
	    if ((p[-j] & 0xc0) != 0x80)
		break;
	if (utf8len_tab[p[-j]] != i + j + 1)
	    return 0;
	return i;
    }

    /* It can't be the first byte if a double-byte when not using DBCS, at the
     * end of the string or the byte can't start a double-byte. */
    if (enc_dbcs == 0 || p[1] == NUL || MB_BYTE2LEN(*p) == 1)
	return 0;

    /* Return 1 when on the lead byte, 0 when on the tail byte. */
    return 1 - dbcs_head_off(base, p);
}

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * Special version of mb_tail_off() for use in ScreenLines[].
 */
    int
dbcs_screen_tail_off(base, p)
    char_u	*base;
    char_u	*p;
{
    /* It can't be the first byte if a double-byte when not using DBCS, at the
     * end of the string or the byte can't start a double-byte.
     * For euc-jp an 0x8e byte always means we have a lead byte in the current
     * cell. */
    if (*p == NUL || p[1] == NUL
	    || (enc_dbcs == DBCS_JPNU && *p == 0x8e)
	    || MB_BYTE2LEN(*p) == 1)
	return 0;

    /* Return 1 when on the lead byte, 0 when on the tail byte. */
    return 1 - dbcs_screen_head_off(base, p);
}
#endif

/*
 * If the cursor moves on an trail byte, set the cursor on the lead byte.
 * Thus it moves left if necessary.
 * Return TRUE when the cursor was adjusted.
 */
    void
mb_adjust_cursor()
{
    mb_adjustpos(&curwin->w_cursor);
}

/*
 * Adjust position "*lp" to point to the first byte of a multi-byte character.
 * If it points to a tail byte it's moved backwards to the head byte.
 */
    void
mb_adjustpos(lp)
    pos_T	*lp;
{
    char_u	*p;

    if (lp->col > 0)
    {
	p = ml_get(lp->lnum);
	lp->col -= (*mb_head_off)(p, p + lp->col);
    }
}

/*
 * Return a pointer to the character before "*p", if there is one.
 */
    char_u *
mb_prevptr(line, p)
    char_u *line;	/* start of the string */
    char_u *p;
{
    if (p > line)
	p = p - (*mb_head_off)(line, p - 1) - 1;
    return p;
}

/*
 * Return the character length of "str".  multi-byte characters counts as one.
 */
    int
mb_charlen(str)
    char_u	*str;
{
    int count;

    if (str == NULL)
	return 0;

    for (count = 0; *str != NUL; count++)
	str += (*mb_ptr2len_check)(str);

    return count;
}

/*
 * Decrement position "lp" by one character, taking care of multi-byte chars.
 */
    int
mb_dec(lp)
    pos_T	*lp;
{
    if (lp->col > 0)		/* still within line */
    {
	--lp->col;
	mb_adjustpos(lp);
	return 0;
    }
    if (lp->lnum > 1)		/* there is a prior line */
    {
	lp->lnum--;
	lp->col = (colnr_T)STRLEN(ml_get(lp->lnum));
	mb_adjustpos(lp);
	return 1;
    }
    return -1;			/* at start of file */
}

/*
 * Try to un-escape a multi-byte character.
 * Used for the "to" and "from" part of a mapping.
 * Return the un-escaped string if it is a multi-byte character, and advance
 * "pp" to just after the bytes that formed it.
 * Return NULL if no multi-byte char was found.
 */
    char_u *
mb_unescape(pp)
    char_u **pp;
{
    static char_u	buf[MB_MAXBYTES + 1];
    int			n, m = 0;
    char_u		*str = *pp;

    /* Must translate K_SPECIAL KS_SPECIAL KE_FILLER to K_SPECIAL and CSI
     * KS_EXTRA KE_CSI to CSI. */
    for (n = 0; str[n] != NUL && m <= MB_MAXBYTES; ++n)
    {
	if (str[n] == K_SPECIAL
		&& str[n + 1] == KS_SPECIAL
		&& str[n + 2] == KE_FILLER)
	{
	    buf[m++] = K_SPECIAL;
	    n += 2;
	}
# ifdef FEAT_GUI
	else if (str[n] == CSI
		&& str[n + 1] == KS_EXTRA
		&& str[n + 2] == (int)KE_CSI)
	{
	    buf[m++] = CSI;
	    n += 2;
	}
# endif
	else if (str[n] == K_SPECIAL
# ifdef FEAT_GUI
		|| str[n] == CSI
# endif
		)
	    break;		/* a special key can't be a multibyte char */
	else
	    buf[m++] = str[n];
	buf[m] = NUL;

	/* Return a multi-byte character if it's found.  An illegal sequence
	 * will result in a 1 here. */
	if ((*mb_ptr2len_check)(buf) > 1)
	{
	    *pp = str + n + 1;
	    return buf;
	}
    }
    return NULL;
}

#if defined(FEAT_CLIPBOARD) || defined(FEAT_GUI) || defined(PROTO)
/*
 * Return TRUE if the character at "row"/"col" on the screen is the left side
 * of a double-width character.
 * Caller must make sure "row" and "col" are not invalid!
 */
    int
mb_lefthalve(row, col)
    int	    row;
    int	    col;
{
#ifdef FEAT_HANGULIN
    if (composing_hangul)
	return TRUE;
#endif
    if (enc_dbcs != 0)
	return dbcs_off2cells(LineOffset[row] + col) > 1;
    if (enc_utf8)
	return (col + 1 < Columns
		&& ScreenLines[LineOffset[row] + col + 1] == 0);
    return FALSE;
}
#endif

#if defined(FEAT_CLIPBOARD) || defined(FEAT_GUI) || defined(PROTO)
/*
 * Correct a position on the screen, if it's the right halve of a double-wide
 * char move it to the left halve.  Returns the corrected column.
 */
    int
mb_fix_col(col, row)
    int		col;
    int		row;
{
    col = check_col(col);
    row = check_row(row);
    if (has_mbyte && ScreenLines != NULL && col > 0
	    && ((enc_dbcs
		    && ScreenLines[LineOffset[row] + col] != NUL
		    && dbcs_screen_head_off(ScreenLines + LineOffset[row],
					 ScreenLines + LineOffset[row] + col))
		|| (enc_utf8 && ScreenLines[LineOffset[row] + col] == 0)))
	--col;
    return col;
}
#endif

/*
 * Skip the Vim specific head of a 'encoding' name.
 */
    char_u *
enc_skip(p)
    char_u	*p;
{
    if (STRNCMP(p, "2byte-", 6) == 0)
	return p + 6;
    if (STRNCMP(p, "8bit-", 5) == 0)
	return p + 5;
    return p;
}

/*
 * Find the canonical name for encoding "enc".
 * When the name isn't recognized, returns "enc" itself, but with all lower
 * case characters and '_' replaced with '-'.
 * Returns an allocated string.  NULL for out-of-memory.
 */
    char_u *
enc_canonize(enc)
    char_u	*enc;
{
    char_u	*r;
    char_u	*p, *s;
    int		i;

    /* copy "enc" to allocted memory, with room for two '-' */
    r = alloc((unsigned)(STRLEN(enc) + 3));
    if (r != NULL)
    {
	/* Make it all lower case and replace '_' with '-'. */
	p = r;
	for (s = enc; *s != NUL; ++s)
	{
	    if (*s == '_')
		*p++ = '-';
	    else
		*p++ = TO_LOWER(*s);
	}
	*p = NUL;

	/* Skip "2byte-" and "8bit-". */
	p = enc_skip(r);

	/* "iso8859" -> "iso-8859" */
	if (STRNCMP(p, "iso8859", 7) == 0)
	{
	    mch_memmove(p + 4, p + 3, STRLEN(p + 2));
	    p[3] = '-';
	}

	/* "iso-8859n" -> "iso-8859-n" */
	if (STRNCMP(p, "iso-8859", 8) == 0 && p[8] != '-')
	{
	    mch_memmove(p + 9, p + 8, STRLEN(p + 7));
	    p[8] = '-';
	}

	/* "latin-N" -> "latinN" */
	if (STRNCMP(p, "latin-", 6) == 0)
	    mch_memmove(p + 5, p + 6, STRLEN(p + 5));

	if (enc_canon_search(p) >= 0)
	{
	    /* canonical name can be used unmodified */
	    if (p != r)
		mch_memmove(r, p, STRLEN(p) + 1);
	}
	else if ((i = enc_alias_search(p)) >= 0)
	{
	    /* alias recognized, get canonical name */
	    vim_free(r);
	    r = vim_strsave((char_u *)enc_canon_table[i].name);
	}
    }
    return r;
}

/*
 * Search for an encoding alias of "name".
 * Returns -1 when not found.
 */
    static int
enc_alias_search(name)
    char_u	*name;
{
    int		i;

    for (i = 0; enc_alias_table[i].name != NULL; ++i)
	if (STRCMP(name, enc_alias_table[i].name) == 0)
	    return enc_alias_table[i].canon;
    return -1;
}

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

/*
 * Return the default value for 'encoding' in an allocated string.
 * Returns NULL when out of memory or no usable default could be found.
 */
    int
enc_default()
{
#ifndef WIN3264
    char	*s;
    char	*p;
    int		i;
#endif
    char	buf[50];
    char_u	*save_enc;
#ifdef WIN3264
    long	acp = GetACP();

    if (acp == 1200)
	STRCPY(buf, "ucs-2le");
    else if (acp == 1252)
	STRCPY(buf, "latin1");
    else
	sprintf(buf, "cp%ld", acp);
#else
# ifdef HAVE_NL_LANGINFO_CODESET
    if ((s = nl_langinfo(CODESET)) == NULL || *s == NUL)
# endif
# if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
	if ((s = setlocale(LC_CTYPE, NULL)) == NULL || *s == NUL)
# endif
	    if ((s = getenv("LC_ALL")) == NULL || *s == NUL)
		if ((s = getenv("LC_CTYPE")) == NULL || *s == NUL)
		    s = getenv("LANG");

    /* The most generic locale format is:
     * language[_territory][.codeset][@modifier][+special][,[sponsor][_revision]]
     * If there is a '.' remove the part before it.
     * if there is something after the codeset, remove it.
     * Make the name lowercase and replace '_' with '-'.
     * Exception: "ja_JP.EUC" == "euc-jp", "zh_CN.EUC" = "euc-cn",
     * "ko_KR.EUC" == "euc-kr"
     */
    if ((p = (char *)vim_strchr((char_u *)s, '.')) != NULL)
    {
	if (p > s + 2 && STRNICMP(p + 1, "EUC", 3) == 0
			&& !isalnum((int)p[4]) && p[4] != '-' && p[-3] == '_')
	{
	    /* copy "XY.EUC" to "euc-XY" to buf[10] */
	    STRCPY(buf + 10, "euc-");
	    buf[14] = p[-2];
	    buf[15] = p[-1];
	    buf[16] = 0;
	    s = buf + 10;
	}
	else
	    s = p + 1;
    }
    for (i = 0; s[i] != NUL && i < sizeof(buf) - 1; ++i)
    {
	if (s[i] == '_' || s[i] == '-')
	    buf[i] = '-';
	else if (isalnum((int)s[i]))
	    buf[i] = TO_LOWER(s[i]);
	else
	    break;
    }
    buf[i] = NUL;
#endif

    /*
     * Try setting 'encoding' and check if the value is valid.
     * If not, go back to the default "latin1".
     */
    save_enc = p_enc;
    p_enc = enc_canonize((char_u *)buf);
    if (p_enc != NULL && mb_init() == NULL)
	return OK;
    vim_free(p_enc);
    p_enc = save_enc;

    return FAIL;
}

# if defined(USE_ICONV) || defined(PROTO)

static char_u *iconv_string __ARGS((iconv_t fd, char_u *str, int slen));

/*
 * Call iconv_open() with a check if iconv() works properly (there are broken
 * versions).
 * Returns (void *)-1 if failed.
 * (should return iconv_t, but that causes problems with prototypes).
 */
    void *
my_iconv_open(to, from)
    char_u	*to;
    char_u	*from;
{
    iconv_t	fd;
#define ICONV_TESTLEN 400
    char_u	tobuf[ICONV_TESTLEN];
    char	*p;
    size_t	tolen;
    static int	iconv_ok = -1;

    if (iconv_ok == FALSE)
	return (void *)-1;	/* detected a broken iconv() previously */

#ifdef DYNAMIC_ICONV
    /* Check if the iconv.dll can be found. */
    if (!iconv_enabled())
	return (void *)-1;
#endif

    fd = iconv_open((char *)enc_skip(to), (char *)enc_skip(from));

    if (fd != (iconv_t)-1 && iconv_ok == -1)
    {
	/*
	 * Do a dummy iconv() call to check if it actually works.  There is a
	 * version of iconv() on Linux that is broken.  We can't ignore it,
	 * because it's wide-spread.  The symptoms are that after outputting
	 * the initial shift state the "to" pointer is NULL and conversion
	 * stops for no apparent reason after about 8160 characters.
	 */
	p = (char *)tobuf;
	tolen = ICONV_TESTLEN;
	(void)iconv(fd, NULL, NULL, &p, &tolen);
	if (p == NULL)
	{
	    iconv_ok = FALSE;
	    iconv_close(fd);
	    fd = (iconv_t)-1;
	}
	else
	    iconv_ok = TRUE;
    }

    return (void *)fd;
}

/*
 * Convert the string "str[slen]" with iconv().
 * Returns the converted string in allocated memory.  NULL for an error.
 */
    static char_u *
iconv_string(fd, str, slen)
    iconv_t	fd;
    char_u	*str;
    int		slen;
{
    const char	*from;
    size_t	fromlen;
    char	*to;
    size_t	tolen;
    size_t	len = 0;
    size_t	done = 0;
    char_u	*result = NULL;
    char_u	*p;
    int		l;

    from = (char *)str;
    fromlen = slen;
    for (;;)
    {
	if (len == 0 || ICONV_ERRNO == E2BIG)
	{
	    /* Allocate enough room for most conversions.  When re-allocating
	     * increase the buffer size. */
	    len = len + fromlen * 2 + 40;
	    p = alloc((unsigned)len);
	    if (p != NULL && done > 0)
		mch_memmove(p, result, done);
	    vim_free(result);
	    result = p;
	    if (result == NULL)	/* out of memory */
		break;
	}

	to = (char *)result + done;
	tolen = len - done - 2;
	if (iconv(fd, &from, &fromlen, &to, &tolen) != (size_t)-1)
	{
	    /* Finished, append a NUL. */
	    *to = NUL;
	    break;
	}
	if (ICONV_ERRNO == EILSEQ)
	{
	    /* Can't convert: insert a '?' and skip a character.  This assumes
	     * conversion from 'encoding' to something else.  In other
	     * situations we don't know what to skip anyway. */
	    *to++ = '?';
	    if ((*mb_ptr2cells)((char_u *)from) > 1)
		*to++ = '?';
	    l = (*mb_ptr2len_check)((char_u *)from);
	    from += l;
	    fromlen -= l;
	}
	else if (ICONV_ERRNO != E2BIG)
	{
	    /* conversion failed */
	    vim_free(result);
	    result = NULL;
	    break;
	}
	/* Not enough room or skipping illegal sequence. */
	done = to - (char *)result;
    }
    return result;
}

#  if defined(DYNAMIC_ICONV) || defined(PROTO)
/*
 * Dynamically load the "iconv.dll" on Win32.
 */

#ifndef DYNAMIC_ICONV	    /* just generating prototypes */
# define HINSTANCE int
#endif
HINSTANCE hIconvDLL = 0;
HINSTANCE hMsvcrtDLL = 0;

#  ifndef DYNAMIC_ICONV_DLL
#   define DYNAMIC_ICONV_DLL "iconv.dll"
#  endif
#  ifndef DYNAMIC_MSVCRT_DLL
#   define DYNAMIC_MSVCRT_DLL "msvcrt.dll"
#  endif

/*
 * Try opening the iconv.dll and return TRUE if iconv() can be used.
 */
    int
iconv_enabled()
{
    if (hIconvDLL != 0 && hMsvcrtDLL != 0)
	return TRUE;
    hIconvDLL = LoadLibrary(DYNAMIC_ICONV_DLL);
    hMsvcrtDLL = LoadLibrary(DYNAMIC_MSVCRT_DLL);
    if (hIconvDLL == 0 || hMsvcrtDLL == 0)
    {
	iconv_end();
	return FALSE;
    }

    *((FARPROC*)&iconv)		= GetProcAddress(hIconvDLL, "libiconv");
    *((FARPROC*)&iconv_open)	= GetProcAddress(hIconvDLL, "libiconv_open");
    *((FARPROC*)&iconv_close)	= GetProcAddress(hIconvDLL, "libiconv_close");
    *((FARPROC*)&iconvctl)	= GetProcAddress(hIconvDLL, "libiconvctl");
    *((FARPROC*)&iconv_errno)	= GetProcAddress(hMsvcrtDLL, "_errno");
    if (iconv == NULL || iconv_open == NULL || iconv_close == NULL
	    || iconvctl == NULL || iconv_errno == NULL)
    {
	iconv_end();
	return FALSE;
    }
    return TRUE;
}

    void
iconv_end()
{
    if (hIconvDLL != 0)
	FreeLibrary(hIconvDLL);
    if (hMsvcrtDLL != 0)
	FreeLibrary(hMsvcrtDLL);
    hIconvDLL = 0;
    hMsvcrtDLL = 0;
}
#  endif /* DYNAMIC_ICONV */
# endif /* USE_ICONV */

#endif /* FEAT_MBYTE */

#if defined(FEAT_XIM) || defined(PROTO)

static int	xim_is_active = FALSE;  /* XIM should be active in the current
					   mode */
static int	xim_has_focus = FALSE;	/* XIM is really being used for Vim */
#ifdef FEAT_GUI_X11
static XIMStyle	input_style;
static int	status_area_enabled = TRUE;
#endif

#ifdef FEAT_GUI_GTK
static int	xim_preediting INIT(= FALSE);	/* XIM in showmode() */
static int	xim_input_style;
static gboolean	use_status_area = 0;
#endif

static int im_xim_str2keycode __ARGS((unsigned int *code, unsigned int *state));
#ifdef FEAT_GUI_GTK
static void im_xim_send_event_imactivate __ARGS((void));
#endif

/*
 * Convert string to keycode and state for XKeyEvent.
 * When string is valid return OK, when invalid return FAIL.
 *
 * See 'imactivatekey' documentation for the format.
 */
    static int
im_xim_str2keycode(code, state)
    unsigned int *code;
    unsigned int *state;
{
    int		retval = OK;
    int		len;
    unsigned	keycode = 0, keystate = 0;
    Window	window;
    Display	*display;
    char_u	*flag_end;
    char_u	*str;

    if (*p_imak != NUL)
    {
	len = STRLEN(p_imak);
	for (flag_end = p_imak + len - 1;
			    flag_end > p_imak && *flag_end != '-'; --flag_end)
	    ;

	/* Parse modifier keys */
	for (str = p_imak; str < flag_end; ++str)
	{
	    switch (*str)
	    {
		case 's': case 'S':
		    keystate |= ShiftMask;
		    break;
		case 'l': case 'L':
		    keystate |= LockMask;
		    break;
		case 'c': case 'C':
		    keystate |= ControlMask;
		    break;
		case '1':
		    keystate |= Mod1Mask;
		    break;
		case '2':
		    keystate |= Mod2Mask;
		    break;
		case '3':
		    keystate |= Mod3Mask;
		    break;
		case '4':
		    keystate |= Mod4Mask;
		    break;
		case '5':
		    keystate |= Mod5Mask;
		    break;
		case '-':
		    break;
		default:
		    retval = FAIL;
	    }
	}
	if (*str == '-')
	    ++str;

	/* Get keycode from string. */
	gui_get_x11_windis(&window, &display);
	keycode = XKeysymToKeycode(display, XStringToKeysym((char *)str));
	if (keycode == 0)
	    retval = FAIL;

	if (code != NULL)
	    *code = keycode;
	if (state != NULL)
	    *state = keystate;
    }
    return retval;
}

#ifdef FEAT_GUI_GTK
    static void
im_xim_send_event_imactivate()
{
    /* Force turn on preedit state by symulate keypress event.
     * Keycode and state is specified by 'imactivatekey'.
     */
    XKeyEvent ev;

    gui_get_x11_windis(&ev.window, &ev.display);
    ev.root = RootWindow(ev.display, DefaultScreen(ev.display));
    ev.subwindow = None;
    ev.time = CurrentTime;
    ev.x = 1;
    ev.y = 1;
    ev.x_root = 1;
    ev.y_root = 1;
    ev.same_screen = 1;
    ev.type = KeyPress;
    if (im_xim_str2keycode(&ev.keycode, &ev.state) == OK)
	XSendEvent(ev.display, ev.window, 1, KeyPressMask, (XEvent*)&ev);
}
#endif

/*
 * Return TRUE if 'imactivatekey' has a valid value.
 */
    int
im_xim_isvalid_imactivate()
{
    return im_xim_str2keycode(NULL, NULL) == OK;
}

/*
 * Switch using XIM on/off.  This is used by the code that changes "State".
 */
    void
im_set_active(active)
    int		active;
{
    if (xic == NULL)
	return;

    /* Remember the active state, it is needed when Vim gets keyboard focus. */
    xim_is_active = active;

    /* When 'imactivatekey' has valid key-string, try to control XIM preedit
     * state.  When 'imactivatekey' has no or invalid string, try old XIM
     * focus control.
     */
    if (*p_imak != NUL)
    {
#ifdef FEAT_GUI_GTK
	/* BASIC STRATEGY:
	 * Destroy old Input Context (XIC), and create new one.  New XIC
	 * would have a state of preedit that is off.  When argument:active
	 * is false, that's all.  Else argument:active is true, send a key
	 * event specified by 'imactivatekey' to activate XIM preedit state.
	 */

	xim_is_active = TRUE; /* Disable old XIM focus control */
	/* If we can monitor preedit state with preedit callback functions,
	 * try least creation of new XIC.
	 */
	if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
	{
	    if (xim_preediting && !active)
	    {
		/* Force turn off preedit state.  With some IM
		 * implementations, we cannot turn off preedit state by
		 * symulate keypress event.  It is why using such a method
		 * that destroy old IC (input context), and create new one.
		 * When create new IC, its preedit state is usually off.
		 */
		xim_set_focus(FALSE);
		gdk_ic_destroy(xic);
		xim_init();
		xim_preediting = FALSE;
	    }
	    else if (!xim_preediting && active)
		im_xim_send_event_imactivate();
	}
	else
	{
	    /* First, force destroy old IC, and create new one.  It
	     * symulates "turning off preedit state".
	     */
	    xim_set_focus(FALSE);
	    gdk_ic_destroy(xic);
	    xim_init();
	    xim_preediting = FALSE;

	    /* 2nd, when requested to activate IM, symulate this by sending
	     * the event.
	     */
	    if (active)
	    {
		im_xim_send_event_imactivate();
		xim_preediting = TRUE;
	    }
	}
#else
# if 0
	/* When had tested kinput2 + canna + Athena GUI version with
	 * 'imactivatekey' is "s-space", im_xim_send_event_imactivate() did not
	 * work correctly.  It just inserted one space.  I don't know why we
	 * couldn't switch state of XIM preediting.  This is reason why these
	 * codes are commented out.
	 */
	/* First, force destroy old IC, and create new one.  It symulates
	 * "turning off preedit state".
	 */
	xim_set_focus(FALSE);
	XDestroyIC(xic);
	xic = NULL;
	xim_init();

	/* 2nd, when requested to activate IM, symulate this by sending the
	 * event.
	 */
	if (active)
	    im_xim_send_event_imactivate();
# endif
#endif
    }
    xim_set_focus(TRUE);
}

/*
 * Adjust using XIM for gaining or losing keyboard focus.  Also called when
 * "xim_is_active" changes.
 */
    void
xim_set_focus(int focus)
{
    if (xic == NULL)
	return;

    /*
     * XIM only gets focus when the Vim window has keyboard focus and XIM has
     * been set active for the current mode.
     */
    if (focus && xim_is_active)
    {
	if (!xim_has_focus)
	{
	    xim_has_focus = TRUE;
#ifdef FEAT_GUI_GTK
	    gdk_im_begin(xic, gui.drawarea->window);
#else
	    XSetICFocus(xic);
#endif
	}
    }
    else
    {
	if (xim_has_focus)
	{
	    xim_has_focus = FALSE;
#ifdef FEAT_GUI_GTK
	    gdk_im_end();
#else
	    XUnsetICFocus(xic);
#endif
	}
    }
}

/*ARGSUSED*/
    void
im_set_position(row, col)
    int		row;
    int		col;
{
    xim_set_preedit();
}

/*
 * Set the XIM to the current cursor position.
 */
    void
xim_set_preedit()
{
    if (xic == NULL)
	return;

    xim_set_focus(TRUE);

#ifdef FEAT_GUI_GTK
    if (gdk_im_ready())
    {
	int		attrmask;
	GdkICAttr	*attr;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;

# ifdef FEAT_XFONTSET
	if ((xim_input_style & (int)GDK_IM_PREEDIT_POSITION)
		&& gui.fontset != NOFONTSET
		&& gui.fontset->type == GDK_FONT_FONTSET)
	{
	    if (!xim_has_focus)
	    {
		if (attr->spot_location.y >= 0)
		{
		    attr->spot_location.x = 0;
		    attr->spot_location.y = -100;
		    attrmask |= (int)GDK_IC_SPOT_LOCATION;
		}
	    }
	    else
	    {
		gint	width, height;

		if (attr->spot_location.x != TEXT_X(gui.col)
		    || attr->spot_location.y != TEXT_Y(gui.row))
		{
		    attr->spot_location.x = TEXT_X(gui.col);
		    attr->spot_location.y = TEXT_Y(gui.row);
		    attrmask |= (int)GDK_IC_SPOT_LOCATION;
		}

		gdk_window_get_size(gui.drawarea->window, &width, &height);
		width -= 2 * gui.border_offset;
		height -= 2 * gui.border_offset;
		if (xim_input_style & (int)GDK_IM_STATUS_AREA)
		    height -= gui.char_height;
		if (attr->preedit_area.width != width
		    || attr->preedit_area.height != height)
		{
		    attr->preedit_area.x = gui.border_offset;
		    attr->preedit_area.y = gui.border_offset;
		    attr->preedit_area.width = width;
		    attr->preedit_area.height = height;
		    attrmask |= (int)GDK_IC_PREEDIT_AREA;
		}

		if (attr->preedit_fontset != gui.current_font)
		{
		    attr->preedit_fontset = gui.current_font;
		    attrmask |= (int)GDK_IC_PREEDIT_FONTSET;
		}
	    }
	}
# endif /* FEAT_XFONTSET */

	if (xim_fg_color < 0)
	{
	    xim_fg_color = gui.def_norm_pixel;
	    xim_bg_color = gui.def_back_pixel;
	}
	if (attr->preedit_foreground.pixel != xim_fg_color)
	{
	    attr->preedit_foreground.pixel = xim_fg_color;
	    attrmask |= (int)GDK_IC_PREEDIT_FOREGROUND;
	}
	if (attr->preedit_background.pixel != xim_bg_color)
	{
	    attr->preedit_background.pixel = xim_bg_color;
	    attrmask |= (int)GDK_IC_PREEDIT_BACKGROUND;
	}

	if (attrmask != 0)
	    gdk_ic_set_attr(xic, attr, (GdkICAttributesType)attrmask);
    }
#else /* FEAT_GUI_GTK */
    {
	XVaNestedList attr_list;
	XRectangle spot_area;
	XPoint over_spot;
	int line_space;

	if (!xim_has_focus)
	{
	    /* hide XIM cursor */
	    over_spot.x = 0;
	    over_spot.y = -100; /* arbitrary invisible position */
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
							    XNSpotLocation,
							    &over_spot,
							    NULL);
	    XSetICValues(xic, XNPreeditAttributes, attr_list, NULL);
	    XFree(attr_list);
	    return;
	}

	if (input_style & XIMPreeditPosition)
	{
	    if (xim_fg_color < 0)
	    {
		xim_fg_color = gui.def_norm_pixel;
		xim_bg_color = gui.def_back_pixel;
	    }
	    over_spot.x = TEXT_X(gui.col);
	    over_spot.y = TEXT_Y(gui.row);
	    spot_area.x = 0;
	    spot_area.y = 0;
	    spot_area.height = gui.char_height * Rows;
	    spot_area.width  = gui.char_width * Columns;
	    line_space = gui.char_height;
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
					    XNSpotLocation, &over_spot,
					    XNForeground, (Pixel) xim_fg_color,
					    XNBackground, (Pixel) xim_bg_color,
					    XNArea, &spot_area,
					    XNLineSpace, line_space,
					    NULL);
	    if (XSetICValues(xic, XNPreeditAttributes, attr_list, NULL))
		EMSG(_("E284: Cannot set IC values"));
	    XFree(attr_list);
	}
    }
#endif /* FEAT_GUI_GTK */
}

/*
 * Set up the status area.
 *
 * This should use a separate Widget, but that seems not possible, because
 * preedit_area and status_area should be set to the same window as for the
 * text input.  Unfortunately this means the status area pollutes the text
 * window...
 */
    void
xim_set_status_area()
{
    if (xic == NULL)
	return;

#ifdef FEAT_GUI_GTK
# if defined(FEAT_XFONTSET)
    if (use_status_area)
    {
	GdkICAttr	*attr;
	int		style;
	gint		width, height;
	GtkWidget	*widget;
	int		attrmask;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;
	style = (int)gdk_ic_get_style(xic);
	if ((style & (int)GDK_IM_STATUS_MASK) == (int)GDK_IM_STATUS_AREA)
	{
	    if (gui.fontset != NOFONTSET
		    && gui.fontset->type == GDK_FONT_FONTSET)
	    {
		widget = gui.mainwin;
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= (int)GDK_IC_STATUS_AREA;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
	    }
	}
	if (attrmask != 0)
	    gdk_ic_set_attr(xic, attr, (GdkICAttributesType)attrmask);
    }
# endif
#else
    {
	XVaNestedList preedit_list = 0, status_list = 0, list = 0;
	XRectangle pre_area, status_area;

	if (input_style & XIMStatusArea)
	{
	    if (input_style & XIMPreeditArea)
	    {
		XRectangle *needed_rect;

		/* to get status_area width */
		status_list = XVaCreateNestedList(0, XNAreaNeeded,
						  &needed_rect, NULL);
		XGetICValues(xic, XNStatusAttributes, status_list, NULL);
		XFree(status_list);

		status_area.width = needed_rect->width;
	    }
	    else
		status_area.width = gui.char_width * Columns;

	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#if defined(FEAT_MENU) && !defined(FEAT_GUI_GTK)
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.height = gui.char_height;
	    status_list = XVaCreateNestedList(0, XNArea, &status_area, NULL);
	}
	else
	{
	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#ifdef FEAT_MENU
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.width = 0;
	    status_area.height = gui.char_height;
	}

	if (input_style & XIMPreeditArea)   /* off-the-spot */
	{
	    pre_area.x = status_area.x + status_area.width;
	    pre_area.y = gui.char_height * Rows + gui.border_offset;
	    pre_area.width = gui.char_width * Columns - pre_area.x;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		pre_area.y += gui.scrollbar_height;
#ifdef FEAT_MENU
	    if (gui.menu_is_active)
		pre_area.y += gui.menu_height;
#endif
	    pre_area.height = gui.char_height;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}
	else if (input_style & XIMPreeditPosition)   /* over-the-spot */
	{
	    pre_area.x = 0;
	    pre_area.y = 0;
	    pre_area.height = gui.char_height * Rows;
	    pre_area.width = gui.char_width * Columns;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}

	if (preedit_list && status_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       XNStatusAttributes, status_list, NULL);
	else if (preedit_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       NULL);
	else if (status_list)
	    list = XVaCreateNestedList(0, XNStatusAttributes, status_list,
				       NULL);
	else
	    list = NULL;

	if (list)
	{
	    XSetICValues(xic, XNVaNestedList, list, NULL);
	    XFree(list);
	}
	if (status_list)
	    XFree(status_list);
	if (preedit_list)
	    XFree(preedit_list);
    }
#endif
}

#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
static char e_xim[] = N_("E285: Failed to create input context");
#endif

#if defined(FEAT_GUI_X11) || defined(PROTO)
# if defined(XtSpecificationRelease) && XtSpecificationRelease >= 6 && !defined(sun)
#  define USE_X11R6_XIM
# endif

static int xim_real_init __ARGS((Window x11_window, Display *x11_display));


#ifdef USE_X11R6_XIM
static void xim_instantiate_cb __ARGS((Display *display, XPointer client_data, XPointer	call_data));
static void xim_destroy_cb __ARGS((XIM im, XPointer client_data, XPointer call_data));

/*ARGSUSED*/
    static void
xim_instantiate_cb(display, client_data, call_data)
    Display	*display;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);
    if (display != x11_display)
	return;

    xim_real_init(x11_window, x11_display);
    gui_set_shellsize(FALSE);
    if (xic != NULL)
	XUnregisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
					 xim_instantiate_cb, NULL);
}

/*ARGSUSED*/
    static void
xim_destroy_cb(im, client_data, call_data)
    XIM		im;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;
    status_area_enabled = FALSE;

    gui_set_shellsize(FALSE);

    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
}
#endif

    void
xim_init()
{
    Window	x11_window;
    Display	*x11_display;

    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;

    if (xim_real_init(x11_window, x11_display))
	return;

    gui_set_shellsize(FALSE);

#ifdef USE_X11R6_XIM
    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
#endif
}

    static int
xim_real_init(x11_window, x11_display)
    Window	x11_window;
    Display	*x11_display;
{
    int		i;
    char	*p,
		*s,
		*ns,
		*end,
		tmp[1024],
		buf[32];
    XIM		xim = NULL;
    XIMStyles	*xim_styles;
    XIMStyle	this_input_style = 0;
    Boolean	found;
    XPoint	over_spot;
    XVaNestedList preedit_list, status_list;

    input_style = 0;
    status_area_enabled = FALSE;

    if (xic != NULL)
	return FALSE;

    if (gui.input_method != NULL && *gui.input_method == NUL)
    {
	strcpy(tmp, gui.input_method);
	for (ns = s = tmp; ns != NULL && *s != NUL;)
	{
	    while (*s && isspace((unsigned char)*s))
		s++;
	    if (!*s)
		break;
	    if ((ns = end = strchr(s, ',')) == NULL)
		end = s + strlen(s);
	    while (isspace((unsigned char)*end))
		end--;
	    *end = NUL;

	    strcpy(buf, "@im=");
	    strcat(buf, s);
	    if ((p = XSetLocaleModifiers(buf)) != NULL
		&& *p != NUL
		&& (xim = XOpenIM(x11_display, NULL, NULL, NULL)) != NULL)
		break;

	    s = ns + 1;
	}
    }

    if (xim == NULL && (p = XSetLocaleModifiers("")) != NULL && *p != NUL)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    /* This is supposed to be useful to obtain characters through
     * XmbLookupString() without really using a XIM. */
    if (xim == NULL && (p = XSetLocaleModifiers("@im=none")) != NULL
								 && *p != NUL)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    if (xim == NULL)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    if (xim == NULL)
    {
	/* Only give this message when verbose is set, because too many people
	 * got this message when they didn't want to use a XIM. */
	if (p_verbose > 0)
	    EMSG(_("E286: Failed to open input method"));
	return FALSE;
    }

#ifdef USE_X11R6_XIM
    {
	XIMCallback destroy_cb;

	destroy_cb.callback = xim_destroy_cb;
	destroy_cb.client_data = NULL;
	if (XSetIMValues(xim, XNDestroyCallback, &destroy_cb, NULL))
	    EMSG(_("E287: Warning: Could not set destroy callback to IM"));
    }
#endif

    if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles)
    {
	EMSG(_("E288: input method doesn't support any style"));
	XCloseIM(xim);
	return FALSE;
    }

    found = False;
    strcpy(tmp, gui.preedit_type);
    for (s = tmp; s && !found; )
    {
	while (*s && isspace((unsigned char)*s))
	    s++;
	if (!*s)
	    break;
	if ((ns = end = strchr(s, ',')) != 0)
	    ns++;
	else
	    end = s + strlen(s);
	while (isspace((unsigned char)*end))
	    end--;
	*end = '\0';

	if (!strcmp(s, "OverTheSpot"))
	    this_input_style = (XIMPreeditPosition | XIMStatusArea);
	else if (!strcmp(s, "OffTheSpot"))
	    this_input_style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp(s, "Root"))
	    this_input_style = (XIMPreeditNothing | XIMStatusNothing);

	for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	{
	    if (this_input_style == xim_styles->supported_styles[i])
	    {
		found = True;
		break;
	    }
	}
	if (!found)
	    for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	    {
		if ((xim_styles->supported_styles[i] & this_input_style)
			== (this_input_style & ~XIMStatusArea))
		{
		    this_input_style &= ~XIMStatusArea;
		    found = True;
		    break;
		}
	    }

	s = ns;
    }
    XFree(xim_styles);

    if (!found)
    {
	/* Only give this message when verbose is set, because too many people
	 * got this message when they didn't want to use a XIM. */
	if (p_verbose > 0)
	    EMSG(_("E289: input method doesn't support my preedit type"));
	XCloseIM(xim);
	return FALSE;
    }

    over_spot.x = TEXT_X(gui.col);
    over_spot.y = TEXT_Y(gui.row);
    input_style = this_input_style;

    /* A crash was reported when trying to pass gui.norm_font as XNFontSet,
     * thus that has been removed.  Hopefully the default works... */
#ifdef FEAT_XFONTSET
    if (gui.fontset != NOFONTSET)
    {
	preedit_list = XVaCreateNestedList(0,
				XNSpotLocation, &over_spot,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				XNFontSet, (XFontSet)gui.fontset,
				NULL);
	status_list = XVaCreateNestedList(0,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				XNFontSet, (XFontSet)gui.fontset,
				NULL);
    }
    else
#endif
    {
	preedit_list = XVaCreateNestedList(0,
				XNSpotLocation, &over_spot,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				NULL);
	status_list = XVaCreateNestedList(0,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				NULL);
    }

    xic = XCreateIC(xim,
		    XNInputStyle, input_style,
		    XNClientWindow, x11_window,
		    XNFocusWindow, gui.wid,
		    XNPreeditAttributes, preedit_list,
		    XNStatusAttributes, status_list,
		    NULL);
    XFree(status_list);
    XFree(preedit_list);
    if (xic != NULL)
    {
	if (input_style & XIMStatusArea)
	{
	    xim_set_status_area();
	    status_area_enabled = TRUE;
	}
	else
	    gui_set_shellsize(FALSE);
    }
    else
    {
	EMSG(_(e_xim));
	XCloseIM(xim);
	return FALSE;
    }

    return TRUE;
}

#endif /* FEAT_GUI_X11 */

#if defined(FEAT_GUI_GTK) || defined(PROTO)

# ifdef FEAT_XFONTSET
static char e_overthespot[] = N_("E290: over-the-spot style requires fontset");
# endif

void
xim_decide_input_style()
{
    /* GDK_IM_STATUS_CALLBACKS was disabled, enabled it to allow Japanese
     * OverTheSpot. */
    int supported_style = (int)GDK_IM_PREEDIT_NONE |
				 (int)GDK_IM_PREEDIT_NOTHING |
				 (int)GDK_IM_PREEDIT_POSITION |
				 (int)GDK_IM_PREEDIT_CALLBACKS |
				 (int)GDK_IM_STATUS_CALLBACKS |
				 (int)GDK_IM_STATUS_AREA |
				 (int)GDK_IM_STATUS_NONE |
				 (int)GDK_IM_STATUS_NOTHING;

    if (!gdk_im_ready())
	xim_input_style = 0;
    else
    {
	if (gtk_major_version > 1
		|| (gtk_major_version == 1
		    && (gtk_minor_version > 2
			|| (gtk_minor_version == 2 && gtk_micro_version >= 3))))
	    use_status_area = TRUE;
	else
	{
	    EMSG(_("E291: Your GTK+ is older than 1.2.3. Status area disabled"));
	    use_status_area = FALSE;
	}
#ifdef FEAT_XFONTSET
	if (gui.fontset == NOFONTSET || gui.fontset->type != GDK_FONT_FONTSET)
#endif
	    supported_style &= ~((int)GDK_IM_PREEDIT_POSITION
						   | (int)GDK_IM_STATUS_AREA);
	if (!use_status_area)
	    supported_style &= ~(int)GDK_IM_STATUS_AREA;
	xim_input_style = (int)gdk_im_decide_style((GdkIMStyle)supported_style);
    }
}

#ifdef FEAT_GUI_GTK
# include <gdk/gdkx.h>
#else
/* Define a few things to be able to generate prototypes while not configured
 * for GTK. */
# define GSList int
# define gboolean int
  typedef int GdkEvent;
# define GdkIC int
#endif

/*ARGSUSED*/
    static void
preedit_start_cbproc(XIC xic, XPointer client_data, XPointer call_data)
{
    xim_preediting = TRUE;
    gui_update_cursor(TRUE, FALSE);
    if (showmode() > 0)
    {
	setcursor();
	out_flush();
    }
}

    static void
xim_back_delete(int n)
{
    char_u str[3];

    str[0] = CSI;
    str[1] = 'k';
    str[2] = 'b';
    while (n-- > 0)
	add_to_input_buf(str, 3);
}

/*
 * Add "str[len]" to the input buffer while escaping CSI bytes.
 */
    static void
add_to_input_buf_csi(char_u *str, int len)
{
    int		i;
    char_u	buf[2];

    for (i = 0; i < len; ++i)
    {
	add_to_input_buf(str + i, 1);
	if (str[i] == CSI)
	{
	    /* Turn CSI into K_CSI. */
	    buf[0] = KS_EXTRA;
	    buf[1] = (int)KE_CSI;
	    add_to_input_buf(buf, 2);
	}
    }
}

static GSList *key_press_event_queue = NULL;
static int preedit_buf_len = 0;
static gboolean processing_queued_event = FALSE;

/*ARGSUSED*/
    static void
preedit_draw_cbproc(XIC xic, XPointer client_data, XPointer call_data)
{
    XIMPreeditDrawCallbackStruct *draw_data;
    XIMText	*text;
    char	*src;
    GSList	*event_queue;

    draw_data = (XIMPreeditDrawCallbackStruct *) call_data;
    text = (XIMText *) draw_data->text;

    if (draw_data->chg_length > 0)
    {
	int bs_cnt;

	if (draw_data->chg_length > preedit_buf_len)
	    bs_cnt = preedit_buf_len;
	else
	    bs_cnt = draw_data->chg_length;
	xim_back_delete(bs_cnt);
	preedit_buf_len -= bs_cnt;
    }
    if (text != NULL && (src = text->string.multi_byte) != NULL)
    {
	int		len = strlen(src);
#ifdef FEAT_MBYTE
	char_u		*buf;
#endif
	GdkWChar	*wstr = NULL;

	wstr = g_new(GdkWChar, len);
	preedit_buf_len += gdk_mbstowcs(wstr, src, len);
	g_free(wstr);
#ifdef FEAT_MBYTE
	if (input_conv.vc_type != CONV_NONE
		&& (buf = string_convert(&input_conv,
						 (char_u *)src, &len)) != NULL)
	{
	    /* Converted from 'termencoding' to 'encoding'. */
	    add_to_input_buf_csi(buf, len);
	    vim_free(buf);
	}
	else
#endif
	    add_to_input_buf_csi((char_u *)src, len);
    }
    event_queue = key_press_event_queue;
    processing_queued_event = TRUE;
    while (event_queue)
    {
	GdkEvent *ev = event_queue->data;
	gboolean *ret;
	gtk_signal_emit_by_name((GtkObject*)gui.mainwin, "key_press_event",
				ev, &ret);
	gdk_event_free(ev);
	event_queue = event_queue->next;
    }
    processing_queued_event = FALSE;
    if (key_press_event_queue)
    {
	g_slist_free(key_press_event_queue);
	key_press_event_queue = NULL;
    }
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static void
preedit_caret_cbproc(XIC xic, XPointer client_data, XPointer call_data)
{
}

/*ARGSUSED*/
    static void
preedit_done_cbproc(XIC xic, XPointer client_data, XPointer call_data)
{
    xim_preediting = FALSE;
    gui_update_cursor(TRUE, FALSE);
    if (showmode() > 0)
    {
	setcursor();
	out_flush();
    }
}

    void
xim_reset(void)
{
    char *text;

    if (xic != NULL)
    {
	text = XmbResetIC(((GdkICPrivate *)xic)->xic);
	if (text != NULL && !(xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS))
	    add_to_input_buf_csi((char_u *)text, strlen(text));
	else
	    preedit_buf_len = 0;
	if (text != NULL)
	    XFree(text);
    }
}

    int
xim_queue_key_press_event(GdkEvent *ev)
{
    if (preedit_buf_len <= 0 || processing_queued_event)
	return FALSE;

    key_press_event_queue = g_slist_append(key_press_event_queue,
                                           gdk_event_copy(ev));
    return TRUE;
}

/*ARGSUSED*/
    static void
preedit_callback_setup(GdkIC *ic)
{
    XIC xxic;
    XVaNestedList preedit_attr;
    XIMCallback preedit_start_cb;
    XIMCallback preedit_draw_cb;
    XIMCallback preedit_caret_cb;
    XIMCallback preedit_done_cb;

    xxic = ((GdkICPrivate*)xic)->xic;
    preedit_start_cb.callback = (XIMProc)preedit_start_cbproc;
    preedit_draw_cb.callback = (XIMProc)preedit_draw_cbproc;
    preedit_caret_cb.callback = (XIMProc)preedit_caret_cbproc;
    preedit_done_cb.callback = (XIMProc)preedit_done_cbproc;
    preedit_attr
	= XVaCreateNestedList (0,
			       XNPreeditStartCallback, &preedit_start_cb,
			       XNPreeditDrawCallback, &preedit_draw_cb,
			       XNPreeditCaretCallback, &preedit_caret_cb,
			       XNPreeditDoneCallback, &preedit_done_cb,
			       0);
    XSetICValues (xxic, XNPreeditAttributes, preedit_attr, 0);
    XFree(preedit_attr);
}

/*ARGSUSED*/
    static void
reset_state_setup(GdkIC *ic)
{
#ifdef USE_X11R6_XIM
    /* don't change the input context when we call reset */
    XSetICValues(((GdkICPrivate*)ic)->xic, XNResetState, XIMPreserveState, 0);
#endif
}

    void
xim_init(void)
{
    xic = NULL;
    xic_attr = NULL;

    if (!gdk_im_ready())
    {
	if (p_verbose > 0)
	    EMSG(_("E292: Input Method Server is not running"));
	return;
    }
    if ((xic_attr = gdk_ic_attr_new()) != NULL)
    {
#ifdef FEAT_XFONTSET
	gint width, height;
#endif
	int		mask;
	GdkColormap	*colormap;
	GdkICAttr	*attr = xic_attr;
	int		attrmask = (int)GDK_IC_ALL_REQ;
	GtkWidget	*widget = gui.drawarea;

	attr->style = (GdkIMStyle)xim_input_style;
	attr->client_window = gui.mainwin->window;

	if ((colormap = gtk_widget_get_colormap(widget)) !=
	    gtk_widget_get_default_colormap())
	{
	    attrmask |= (int)GDK_IC_PREEDIT_COLORMAP;
	    attr->preedit_colormap = colormap;
	}
	attrmask |= (int)GDK_IC_PREEDIT_FOREGROUND;
	attrmask |= (int)GDK_IC_PREEDIT_BACKGROUND;
	attr->preedit_foreground = widget->style->fg[GTK_STATE_NORMAL];
	attr->preedit_background = widget->style->base[GTK_STATE_NORMAL];

#ifdef FEAT_XFONTSET
	if ((xim_input_style & (int)GDK_IM_PREEDIT_MASK)
					      == (int)GDK_IM_PREEDIT_POSITION)
	{
	    if (gui.fontset == NOFONTSET
		    || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG(_(e_overthespot));
	    }
	    else
	    {
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= (int)GDK_IC_PREEDIT_POSITION_REQ;
		attr->spot_location.x = TEXT_X(0);
		attr->spot_location.y = TEXT_Y(0);
		attr->preedit_area.x = gui.border_offset;
		attr->preedit_area.y = gui.border_offset;
		attr->preedit_area.width = width - 2*gui.border_offset;
		attr->preedit_area.height = height - 2*gui.border_offset;
		attr->preedit_fontset = gui.fontset;
	    }
	}

	if ((xim_input_style & (int)GDK_IM_STATUS_MASK)
						   == (int)GDK_IM_STATUS_AREA)
	{
	    if (gui.fontset == NOFONTSET
		    || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG(_(e_overthespot));
	    }
	    else
	    {
		gdk_window_get_size(gui.mainwin->window, &width, &height);
		attrmask |= (int)GDK_IC_STATUS_AREA_REQ;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
		attr->status_fontset = gui.fontset;
	    }
	}
	else if ((xim_input_style & (int)GDK_IM_STATUS_MASK)
					      == (int)GDK_IM_STATUS_CALLBACKS)
	{
	    /* FIXME */
	}
#endif

	xic = gdk_ic_new(attr, (GdkICAttributesType)attrmask);

	if (xic == NULL)
	    EMSG(_(e_xim));
	else
	{
	    mask = (int)gdk_window_get_events(widget->window);
	    mask |= (int)gdk_ic_get_events(xic);
	    gdk_window_set_events(widget->window, (GdkEventMask)mask);
	    if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
		preedit_callback_setup(xic);
	    reset_state_setup(xic);
	}
    }
}
#endif /* FEAT_GUI_GTK */

    int
xim_get_status_area_height(void)
{
#ifdef FEAT_GUI_GTK
    if (xim_input_style & (int)GDK_IM_STATUS_AREA)
	return gui.char_height;
#else
    if (status_area_enabled)
	return gui.char_height;
#endif
    return 0;
}

/*
 * Get IM status.  When IM is on, return TRUE.  Else return FALSE.
 */
    int
im_get_status()
{
#ifdef FEAT_GUI_GTK
    if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
	return xim_preediting;
#endif
    return xim_has_focus;
}

#endif /* FEAT_XIM */

#if defined(FEAT_MBYTE) || defined(PROTO)

/*
 * Setup "vcp" for conversion from "from" to "to".
 * The names must have been made canonical with enc_canonize().
 * Note: cannot be used for conversion from/to ucs-2 and ucs-4 (will use utf-8
 * instead).
 */
    void
convert_setup(vcp, from, to)
    vimconv_T	*vcp;
    char_u	*from;
    char_u	*to;
{
    int		from_prop;
    int		to_prop;

    /* Reset to no conversion. */
    vcp->vc_type = CONV_NONE;
    vcp->vc_factor = 1;
# ifdef USE_ICONV
    if (vcp->vc_fd != (iconv_t)-1)
    {
	iconv_close(vcp->vc_fd);
	vcp->vc_fd = (iconv_t)-1;
    }
# endif

    /* No conversion when one of the names is empty or they are equal. */
    if (*from == NUL || *to == NUL || STRCMP(from, to) == 0)
	return;

    from_prop = enc_canon_props(from);
    to_prop = enc_canon_props(to);
    if ((from_prop & ENC_LATIN1) && (to_prop & ENC_UNICODE))
    {
	/* Internal latin1 -> utf-8 conversion. */
	vcp->vc_type = CONV_TO_UTF8;
	vcp->vc_factor = 2;	/* up to twice as long */
    }
    else if ((from_prop & ENC_UNICODE) && (to_prop & ENC_LATIN1))
    {
	/* Internal utf-8 -> latin1 conversion. */
	vcp->vc_type = CONV_TO_LATIN1;
    }
# ifdef USE_ICONV
    else
    {
	/* Use iconv() for conversion. */
	vcp->vc_fd = (iconv_t)my_iconv_open(
		(to_prop & ENC_UNICODE) ? (char_u *)"utf-8" : to,
		(from_prop & ENC_UNICODE) ? (char_u *)"utf-8" : from);
	if (vcp->vc_fd != (iconv_t)-1)
	{
	    vcp->vc_type = CONV_ICONV;
	    vcp->vc_factor = 4;	/* could be longer too... */
	}
    }
# endif
}

/*
 * Do conversion on typed input characters in-place.
 * Returns the length after conversion.
 */
    int
convert_input(ptr, len, maxlen)
    char_u	*ptr;
    int		len;
    int		maxlen;
{
    char_u	*d;
    int		l;

    d = string_convert(&input_conv, ptr, &len);
    if (d != NULL)
    {
	l = (int)STRLEN(d);
	if (l <= maxlen)
	{
	    mch_memmove(ptr, d, l);
	    len = l;
	}
	vim_free(d);
    }
    return len;
}

/*
 * Convert text "ptr[*lenp]" according to "vcp".
 * Returns the result in allocated memory and sets "*lenp".
 * When "lenp" is NULL, use NUL terminated strings.
 * When something goes wrong, NULL is returned.
 */
    char_u *
string_convert(vcp, ptr, lenp)
    vimconv_T	*vcp;
    char_u	*ptr;
    int		*lenp;
{
    char_u	*retval = NULL;
    char_u	*d;
    int		len;
    int		i;
    int		l;
    int		c;

    if (lenp == NULL)
	len = (int)STRLEN(ptr);
    else
	len = *lenp;

    switch (vcp->vc_type)
    {
	case CONV_TO_UTF8:	/* latin1 to utf-8 conversion */
	    retval = alloc(len * 2 + 1);
	    if (retval == NULL)
		break;
	    d = retval;
	    for (i = 0; i < len; ++i)
	    {
		if (ptr[i] < 0x80)
		    *d++ = ptr[i];
		else
		{
		    *d++ = 0xc0 + ((unsigned)ptr[i] >> 6);
		    *d++ = 0x80 + (ptr[i] & 0x3f);
		}
	    }
	    *d = NUL;
	    if (lenp != NULL)
		*lenp = (int)(d - retval);
	    break;

	case CONV_TO_LATIN1:	/* utf-8 to latin1 conversion */
	    retval = alloc(len + 1);
	    if (retval == NULL)
		break;
	    d = retval;
	    for (i = 0; i < len; ++i)
	    {
		l = utf_ptr2len_check(ptr + i);
		if (l <= 1)
		    *d++ = ptr[i];
		else
		{
		    c = utf_ptr2char(ptr + i);
		    if (!utf_iscomposing(c))	/* skip composing chars */
		    {
			if (c < 0x100)
			    *d++ = c;
			else
			{
			    *d++ = 0xbf;
			    if (utf_char2cells(c) > 1)
				*d++ = '?';
			}
		    }
		    i += l - 1;
		}
	    }
	    *d = NUL;
	    if (lenp != NULL)
		*lenp = (int)(d - retval);
	    break;

# ifdef USE_ICONV
	case CONV_ICONV:	/* conversion with output_conv.vc_fd */
	    retval = iconv_string(vcp->vc_fd, ptr, len);
	    if (retval != NULL && lenp != NULL)
		*lenp = (int)STRLEN(retval);
# endif
    }

    return retval;
}
#endif
