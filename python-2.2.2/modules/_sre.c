//20180406
static char copyright[] =
    " SRE 2.2.1 Copyright (c) 1997-2001 by Secret Labs AB ";

#include "python.h"
#include "structmember.h"

#include "sre.h"

#include <ctype.h>

#define SRE_MODULE "sre"

#undef VERBOSE

#define HAVE_UNICODE

#define USE_FAST_SEARCH

#undef USE_INLINE

#undef USE_BUILTIN_COPY

#define LOCAL(type) static type

#define SRE_ERROR_ILLEGAL -1 
#define SRE_ERROR_STATE -2 
#define SRE_ERROR_RECURSION_LIMIT -3 
#define SRE_ERROR_MEMORY -9

#define TRACE(v)

#define SRE_DIGIT_MASK 1
#define SRE_SPACE_MASK 2
#define SRE_LINEBREAK_MASK 4
#define SRE_ALNUM_MASK 8
#define SRE_WORD_MASK 16

static char sre_char_info[128] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 6, 2,
2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 25, 25, 25, 25, 25, 25, 25,
25, 25, 0, 0, 0, 0, 0, 0, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0,
0, 0, 16, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0, 0, 0, 0 };

static char sre_char_lower[128] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
120, 121, 122, 123, 124, 125, 126, 127 };

#define SRE_IS_DIGIT(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_DIGIT_MASK) : 0)
#define SRE_IS_SPACE(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_SPACE_MASK) : 0)
#define SRE_IS_LINEBREAK(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_LINEBREAK_MASK) : 0)
#define SRE_IS_ALNUM(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_ALNUM_MASK) : 0)
#define SRE_IS_WORD(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_WORD_MASK) : 0)

static unsigned int sre_lower(unsigned int ch)
{
    return ((ch) < 128 ? sre_char_lower[ch] : ch);
}

#define SRE_LOC_IS_DIGIT(ch) ((ch) < 256 ? isdigit((ch)) : 0)
#define SRE_LOC_IS_SPACE(ch) ((ch) < 256 ? isspace((ch)) : 0)
#define SRE_LOC_IS_LINEBREAK(ch) ((ch) == '\n')
#define SRE_LOC_IS_ALNUM(ch) ((ch) < 256 ? isalnum((ch)) : 0)
#define SRE_LOC_IS_WORD(ch) (SRE_LOC_IS_ALNUM((ch)) || (ch) == '_')

static unsigned int sre_lower_locale(unsigned int ch)
{
    return ((ch) < 256 ? tolower((ch)) : ch);
}

#define SRE_UNI_IS_DIGIT(ch) Py_UNICODE_ISDIGIT((Py_UNICODE)(ch))
#define SRE_UNI_IS_SPACE(ch) Py_UNICODE_ISSPACE((Py_UNICODE)(ch))
#define SRE_UNI_IS_LINEBREAK(ch) Py_UNICODE_ISLINEBREAK((Py_UNICODE)(ch))
#define SRE_UNI_IS_ALNUM(ch) Py_UNICODE_ISALNUM((Py_UNICODE)(ch))
#define SRE_UNI_IS_WORD(ch) (SRE_UNI_IS_ALNUM((ch)) || (ch) == '_')

static unsigned int sre_lower_unicode(unsigned int ch)
{
    return (unsigned int) Py_UNICODE_TOLOWER((Py_UNICODE)(ch));
}

LOCAL(int) sre_category(SRE_CODE category, unsigned int ch)
{
    switch (category) {

    case SRE_CATEGORY_DIGIT:
        return SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_NOT_DIGIT:
        return !SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_SPACE:
        return SRE_IS_SPACE(ch);
    case SRE_CATEGORY_NOT_SPACE:
        return !SRE_IS_SPACE(ch);
    case SRE_CATEGORY_WORD:
        return SRE_IS_WORD(ch);
    case SRE_CATEGORY_NOT_WORD:
        return !SRE_IS_WORD(ch);
    case SRE_CATEGORY_LINEBREAK:
        return SRE_IS_LINEBREAK(ch);
    case SRE_CATEGORY_NOT_LINEBREAK:
        return !SRE_IS_LINEBREAK(ch);

    case SRE_CATEGORY_LOC_WORD:
        return SRE_LOC_IS_WORD(ch);
    case SRE_CATEGORY_LOC_NOT_WORD:
        return !SRE_LOC_IS_WORD(ch);

    case SRE_CATEGORY_UNI_DIGIT:
        return SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_NOT_DIGIT:
        return !SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_SPACE:
        return SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_NOT_SPACE:
        return !SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_WORD:
        return SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_NOT_WORD:
        return !SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_LINEBREAK:
        return SRE_UNI_IS_LINEBREAK(ch);
    case SRE_CATEGORY_UNI_NOT_LINEBREAK:
        return !SRE_UNI_IS_LINEBREAK(ch);
    }
    return 0;
}

static void mark_fini(SRE_STATE* state)
{
    if (state->mark_stack) {
        free(state->mark_stack);
        state->mark_stack = NULL;
    }
    state->mark_stack_size = state->mark_stack_base = 0;
}

static int mark_save(SRE_STATE* state, int lo, int hi)
{
    void* stack;
    int size;
    int minsize, newsize;

    if (hi <= lo)
        return 0;

    size = (hi - lo) + 1;

    newsize = state->mark_stack_size;
    minsize = state->mark_stack_base + size;

    if (newsize < minsize) 
	{
        if (!newsize) 
		{
            newsize = 512;
            if (newsize < minsize)
            {
				newsize = minsize;
            }
			TRACE(("allocate stack %d\n", newsize));
            stack = malloc(sizeof(void*) * newsize);
        } 
		else 
		{
            while (newsize < minsize)
            {
				newsize += newsize;
            }
			TRACE(("grow stack to %d\n", newsize));
            stack = realloc(state->mark_stack, sizeof(void*) * newsize);
        }
        if (!stack) 
		{
            mark_fini(state);
            return SRE_ERROR_MEMORY;
        }
        state->mark_stack = stack;
        state->mark_stack_size = newsize;
    }

    TRACE(("copy %d:%d to %d (%d)\n", lo, hi, state->mark_stack_base, size));

    memcpy(state->mark_stack + state->mark_stack_base, state->mark + lo,
           size * sizeof(void*));

    state->mark_stack_base += size;

    return 0;
}

static int mark_restore(SRE_STATE* state, int lo, int hi)
{
    int size;

    if (hi <= lo)
	{
        return 0;
	}

    size = (hi - lo) + 1;

    state->mark_stack_base -= size;

    TRACE(("copy %d:%d from %d\n", lo, hi, state->mark_stack_base));

    memcpy(state->mark + lo, state->mark_stack + state->mark_stack_base,
           size * sizeof(void*));

    return 0;
}

#define SRE_CHAR unsigned char
#define SRE_AT sre_at
#define SRE_COUNT sre_count
#define SRE_CHARSET sre_charset
#define SRE_INFO sre_info
#define SRE_MATCH sre_match
#define SRE_SEARCH sre_search
#define SRE_LITERAL_TEMPLATE sre_literal_template

#define SRE_RECURSIVE



LOCAL(int) SRE_AT(SRE_STATE* state, SRE_CHAR* ptr, SRE_CODE at)
{
    int this, that;

    switch (at) 
	{
    case SRE_AT_BEGINNING:
    case SRE_AT_BEGINNING_STRING:
        return ((void*) ptr == state->beginning);

    case SRE_AT_BEGINNING_LINE:
        return ((void*) ptr == state->beginning ||
                SRE_IS_LINEBREAK((int) ptr[-1]));

    case SRE_AT_END:
        return (((void*) (ptr+1) == state->end &&
                 SRE_IS_LINEBREAK((int) ptr[0])) ||
                ((void*) ptr == state->end));

    case SRE_AT_END_LINE:
        return ((void*) ptr == state->end ||
                SRE_IS_LINEBREAK((int) ptr[0]));

    case SRE_AT_END_STRING:
        return ((void*) ptr == state->end);

    case SRE_AT_BOUNDARY:
        if (state->beginning == state->end)
		{
            return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this == that;

    case SRE_AT_LOC_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_LOC_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return this == that;

    case SRE_AT_UNI_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_UNI_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return this == that;
    }

    return 0;
}

LOCAL(int) SRE_CHARSET(SRE_CODE* set, SRE_CODE ch)
{
    int ok = 1;

    for (;;) 
	{
        switch (*set++) 
		{
        case SRE_OP_LITERAL:
            if (ch == set[0])
            {
				return ok;
            }
			set++;
            break;

        case SRE_OP_RANGE:
            if (set[0] <= ch && ch <= set[1])
            {
				return ok;
            }
			set += 2;
            break;

        case SRE_OP_CHARSET:
            if (ch < 256 && (set[ch >> 4] & (1 << (ch & 15))))
            {
				return ok;
            }
			set += 16;
            break;

        case SRE_OP_BIGCHARSET:
        {
            int count, block;
            count = *(set++);
            block = ((unsigned char*)set)[ch >> 8];
            set += 128;
            if (set[block*16 + ((ch & 255)>>4)] & (1 << (ch & 15)))
            {
				return ok;
            }
			set += count*16;
            break;
        }

        case SRE_OP_CATEGORY:
            if (sre_category(set[0], (int) ch))
            {
				return ok;
            }
			set += 1;
            break;

        case SRE_OP_NEGATE:
            ok = !ok;
            break;

        case SRE_OP_FAILURE:
            return !ok;

        default:
            return 0;
        }
    }
}

LOCAL(int) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level);

LOCAL(int) SRE_COUNT(SRE_STATE* state, SRE_CODE* pattern, int maxcount, int level)
{
    SRE_CODE chr;
    SRE_CHAR* ptr = state->ptr;
    SRE_CHAR* end = state->end;
    int i;

    if (maxcount < end - ptr && maxcount != 65535)
    {
		end = ptr + maxcount;
	}

    switch (pattern[0]) 
	{
    case SRE_OP_ANY:
        TRACE(("|%p|%p|COUNT ANY\n", pattern, ptr));
        while (ptr < end && !SRE_IS_LINEBREAK(*ptr))
        {
			ptr++;
        }
		break;

    case SRE_OP_ANY_ALL:
        TRACE(("|%p|%p|COUNT ANY_ALL\n", pattern, ptr));
        ptr = end;
        break;

    case SRE_OP_LITERAL:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr == chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_LITERAL_IGNORE:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) == chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_NOT_LITERAL:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr != chr)
        {
			ptr++;
        }
		break;
                
    case SRE_OP_NOT_LITERAL_IGNORE:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) != chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_IN:
        TRACE(("|%p|%p|COUNT IN\n", pattern, ptr));
        while (ptr < end && SRE_CHARSET(pattern + 2, *ptr))
        {
			ptr++;
        }
		break;

    default:
        TRACE(("|%p|%p|COUNT SUBPATTERN\n", pattern, ptr));
        while ((SRE_CHAR*) state->ptr < end) 
		{
            i = SRE_MATCH(state, pattern, level);
            if (i < 0)
            {
				return i;
            }
			if (!i)
            {
				break;
			}
        }
        TRACE(("|%p|%p|COUNT %d\n", pattern, ptr,
               (SRE_CHAR*) state->ptr - ptr));
        return (SRE_CHAR*) state->ptr - ptr;
    }

    TRACE(("|%p|%p|COUNT %d\n", pattern, ptr, ptr - (SRE_CHAR*) state->ptr));
    return ptr - (SRE_CHAR*) state->ptr;
}

LOCAL(int) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level)
{
    SRE_CHAR* end = state->end;
    SRE_CHAR* ptr = state->ptr;
    int i, count;
    SRE_REPEAT* rp;
    int lastmark;
    SRE_CODE chr;

    SRE_REPEAT rep; 

    TRACE(("|%p|%p|ENTER %d\n", pattern, ptr, level));

    if (level % 10 == 0 && PyOS_CheckStack())
    {
		return SRE_ERROR_RECURSION_LIMIT;
	}

    if (pattern[0] == SRE_OP_INFO) 
	{
        if (pattern[3] && (end - ptr) < pattern[3]) 
		{
            TRACE(("reject (got %d chars, need %d)\n",
                   (end - ptr), pattern[3]));
            return 0;
        }
        pattern += pattern[1] + 1;
    }

    for (;;) 
	{
        switch (*pattern++) 
		{
        case SRE_OP_FAILURE:
            TRACE(("|%p|%p|FAILURE\n", pattern, ptr));
            return 0;

        case SRE_OP_SUCCESS:
            TRACE(("|%p|%p|SUCCESS\n", pattern, ptr));
            state->ptr = ptr;
            return 1;

        case SRE_OP_AT:
            TRACE(("|%p|%p|AT %d\n", pattern, ptr, *pattern));
            if (!SRE_AT(state, ptr, *pattern))
            {
				return 0;
            }
			pattern++;
            break;

        case SRE_OP_CATEGORY:
            TRACE(("|%p|%p|CATEGORY %d\n", pattern, ptr, *pattern));
            if (ptr >= end || !sre_category(pattern[0], ptr[0]))
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_LITERAL:
            TRACE(("|%p|%p|LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] != pattern[0])
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL:
            TRACE(("|%p|%p|NOT_LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] == pattern[0])
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_ANY:
            TRACE(("|%p|%p|ANY\n", pattern, ptr));
            if (ptr >= end || SRE_IS_LINEBREAK(ptr[0]))
            {
				return 0;
            }
			ptr++;
            break;

        case SRE_OP_ANY_ALL:
            TRACE(("|%p|%p|ANY_ALL\n", pattern, ptr));
            if (ptr >= end)
            {
				return 0;
            }
			ptr++;
            break;

        case SRE_OP_IN:
            TRACE(("|%p|%p|IN\n", pattern, ptr));
            if (ptr >= end || !SRE_CHARSET(pattern + 1, *ptr))
            {
				return 0;
            }
			pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_GROUPREF:
            TRACE(("|%p|%p|GROUPREF %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                {
					return 0;
                }
				while (p < e) 
				{
                    if (ptr >= end || *ptr != *p)
                    {
						return 0;
                    }
					p++; 
					ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_GROUPREF_IGNORE:
            TRACE(("|%p|%p|GROUPREF_IGNORE %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                {
					return 0;
                }
				while (p < e) 
				{
                    if (ptr >= end ||
                        state->lower(*ptr) != state->lower(*p))
                    {
						return 0;
                    }
					p++; 
					ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_LITERAL_IGNORE:
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n", pattern, ptr, pattern[0]));
            if (ptr >= end ||
                state->lower(*ptr) != state->lower(*pattern))
			{
                return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n", pattern, ptr, *pattern));
            if (ptr >= end ||
                state->lower(*ptr) == state->lower(*pattern))
			{
                return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_IN_IGNORE:
            TRACE(("|%p|%p|IN_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE_CHARSET(pattern + 1, (SRE_CODE) state->lower(*ptr)))
			{
                return 0;
            }
			pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_MARK:
            TRACE(("|%p|%p|MARK %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            if (i & 1)
			{
                state->lastindex = i/2 + 1;
            }
			if (i > state->lastmark)
            {
				state->lastmark = i;
            }
			state->mark[i] = ptr;
            pattern++;
            break;

        case SRE_OP_JUMP:
        case SRE_OP_INFO:
            TRACE(("|%p|%p|JUMP %d\n", pattern, ptr, pattern[0]));
            pattern += pattern[0];
            break;

        case SRE_OP_ASSERT:
            TRACE(("|%p|%p|ASSERT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr < state->beginning)
            {
				return 0;
            }
			i = SRE_MATCH(state, pattern + 2, level + 1);
            if (i <= 0)
            {
				return i;
            }
			pattern += pattern[0];
            break;

        case SRE_OP_ASSERT_NOT:
            TRACE(("|%p|%p|ASSERT_NOT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr >= state->beginning) 
			{
                i = SRE_MATCH(state, pattern + 2, level + 1);
                if (i < 0)
                {
					return i;
                }
				if (i)
                {
					return 0;
				}
            }
            pattern += pattern[0];
            break;

        case SRE_OP_BRANCH:
            TRACE(("|%p|%p|BRANCH\n", pattern, ptr));
            lastmark = state->lastmark;
            for (; pattern[0]; pattern += pattern[0]) 
			{
                if (pattern[1] == SRE_OP_LITERAL &&
                    (ptr >= end || (SRE_CODE) *ptr != pattern[2]))
				{
                    continue;
                }
				if (pattern[1] == SRE_OP_IN &&
                    (ptr >= end || !SRE_CHARSET(pattern + 3, (SRE_CODE) *ptr)))
				{
                    continue;
                }
				state->ptr = ptr;
                i = SRE_MATCH(state, pattern + 1, level + 1);
                if (i)
				{
                    return i;
                }
				if (state->lastmark > lastmark) 
				{
                    memset(
                        state->mark + lastmark + 1, 0,
                        (state->lastmark - lastmark) * sizeof(void*)
                        );
                    state->lastmark = lastmark;
                }
            }
            return 0;

        case SRE_OP_REPEAT_ONE:
            TRACE(("|%p|%p|REPEAT_ONE %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            if (ptr + pattern[1] > end)
            {
				return 0;
			}

            state->ptr = ptr;

            count = SRE_COUNT(state, pattern + 3, pattern[2], level + 1);
            if (count < 0)
			{
                return count;
			}

            ptr += count;

            if (count < (int) pattern[1])
            {
				return 0;
			}

            if (pattern[pattern[0]] == SRE_OP_SUCCESS) 
			{
                state->ptr = ptr;
                return 1;

            } 
			else if (pattern[pattern[0]] == SRE_OP_LITERAL) 
			{
                chr = pattern[pattern[0]+1];
                for (;;) 
				{
                    while (count >= (int) pattern[1] &&
                           (ptr >= end || *ptr != chr)) 
					{
                        ptr--;
                        count--;
                    }
                    if (count < (int) pattern[1])
                    {
						break;
                    }
					state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
					{
                        return i;
                    }
					ptr--;
                    count--;
                }
            } 
			else 
			{
                lastmark = state->lastmark;
                while (count >= (int) pattern[1]) 
				{
                    state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
					{
                        return i;
                    }
					ptr--;
                    count--;
                    if (state->lastmark > lastmark) 
					{
                        memset(
                            state->mark + lastmark + 1, 0,
                            (state->lastmark - lastmark) * sizeof(void*)
                            );
                        state->lastmark = lastmark;
                    }
                }
            }
            return 0;

        case SRE_OP_REPEAT:
            TRACE(("|%p|%p|REPEAT %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            rep.count = -1;
            rep.pattern = pattern;

            rep.prev = state->repeat;
            state->repeat = &rep;

            state->ptr = ptr;
            i = SRE_MATCH(state, pattern + pattern[0], level + 1);

            state->repeat = rep.prev;

            return i;

        case SRE_OP_MAX_UNTIL:
            rp = state->repeat;
            if (!rp)
			{
                return SRE_ERROR_STATE;
			}

            state->ptr = ptr;

            count = rp->count + 1;

            TRACE(("|%p|%p|MAX_UNTIL %d\n", pattern, ptr, count));

            if (count < rp->pattern[1]) 
			{
                rp->count = count;
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				rp->count = count - 1;
                state->ptr = ptr;
                return 0;
            }

            if (count < rp->pattern[2] || rp->pattern[2] == 65535) 
			{
                rp->count = count;
                lastmark = state->lastmark;
                i = mark_save(state, 0, lastmark);
                if (i < 0)
                {
					return i;
                }
				i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				i = mark_restore(state, 0, lastmark);
                state->lastmark = lastmark;
                if (i < 0)
                {
					return i;
                }
				rp->count = count - 1;
                state->ptr = ptr;
            }
            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i)
            {
				return i;
            }
			state->repeat = rp;
            state->ptr = ptr;
            return 0;

        case SRE_OP_MIN_UNTIL:
            rp = state->repeat;
            if (!rp)
			{
                return SRE_ERROR_STATE;
			}

            count = rp->count + 1;

            TRACE(("|%p|%p|MIN_UNTIL %d %p\n", pattern, ptr, count,
                   rp->pattern));

            state->ptr = ptr;

            if (count < rp->pattern[1]) 
			{
                rp->count = count;
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				rp->count = count-1;
                state->ptr = ptr;
                return 0;
            }

            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i)
			{
                return i;
			}

            state->ptr = ptr;
            state->repeat = rp;

            if (count >= rp->pattern[2] && rp->pattern[2] != 65535)
            {
				return 0;
			}

            rp->count = count;
            i = SRE_MATCH(state, rp->pattern + 3, level + 1);
            if (i)
            {
				return i;
            }
			rp->count = count - 1;
            state->ptr = ptr;
            return 0;

        default:
            TRACE(("|%p|%p|UNKNOWN %d\n", pattern, ptr, pattern[-1]));
            return SRE_ERROR_ILLEGAL;
        }
    }
}

LOCAL(int) SRE_SEARCH(SRE_STATE* state, SRE_CODE* pattern)
{
    SRE_CHAR* ptr = state->start;
    SRE_CHAR* end = state->end;
    int status = 0;
    int prefix_len = 0;
    int prefix_skip = 0;
    SRE_CODE* prefix = NULL;
    SRE_CODE* charset = NULL;
    SRE_CODE* overlap = NULL;
    int flags = 0;

    if (pattern[0] == SRE_OP_INFO) 
	{
        flags = pattern[2];

        if (pattern[3] > 0) 
		{
            end -= pattern[3]-1;
            if (end <= ptr)
            {
				end = ptr + 1;
			}
        }

        if (flags & SRE_INFO_PREFIX) 
		{
            prefix_len = pattern[5];
            prefix_skip = pattern[6];
            prefix = pattern + 7;
            overlap = prefix + prefix_len - 1;
        } 
		else if (flags & SRE_INFO_CHARSET)
		{
            charset = pattern + 5;
		}

        pattern += 1 + pattern[1];
    }

    TRACE(("prefix = %p %d %d\n", prefix, prefix_len, prefix_skip));
    TRACE(("charset = %p\n", charset));

    if (prefix_len > 1) 
	{
        int i = 0;
        end = state->end;
        while (ptr < end) 
		{
            for (;;) 
			{
                if ((SRE_CODE) ptr[0] != prefix[i]) 
				{
                    if (!i)
                    {
						break;
                    }
					else
                    {
						i = overlap[i];
					}
                } 
				else 
				{
                    if (++i == prefix_len) 
					{
                        TRACE(("|%p|%p|SEARCH SCAN\n", pattern, ptr));
                        state->start = ptr + 1 - prefix_len;
                        state->ptr = ptr + 1 - prefix_len + prefix_skip;
                        if (flags & SRE_INFO_LITERAL)
                        {
							return 1;
                        }
						status = SRE_MATCH(state, pattern + 2*prefix_skip, 1);
                        if (status != 0)
						{
                            return status;
                        }
						i = overlap[i];
                    }
                    break;
                }
                
            }
            ptr++;
        }
        return 0;
    }

    if (pattern[0] == SRE_OP_LITERAL) 
	{
        SRE_CODE chr = pattern[1];
        end = state->end;
        for (;;) 
		{
            while (ptr < end && (SRE_CODE) ptr[0] != chr)
            {
				ptr++;
            }
			if (ptr == end)
            {
				return 0;
            }
			TRACE(("|%p|%p|SEARCH LITERAL\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ++ptr;
            if (flags & SRE_INFO_LITERAL)
            {
				return 1;
            }
			status = SRE_MATCH(state, pattern + 2, 1);
            if (status != 0)
			{
                break;
			}
        }
    } 
	else if (charset) 
	{
        end = state->end;
        for (;;) 
		{
            while (ptr < end && !SRE_CHARSET(charset, ptr[0]))
            {
				ptr++;
            }
			if (ptr == end)
            {
				return 0;
            }
			TRACE(("|%p|%p|SEARCH CHARSET\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
            {
				break;
			}
            ptr++;
        }
    } 
	else
    {
		while (ptr <= end) 
		{
            TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
            state->start = state->ptr = ptr++;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
            {
				break;
			}
        }
	}
    return status;
}
    
LOCAL(int) SRE_LITERAL_TEMPLATE(SRE_CHAR* ptr, int len)
{
    while (len-- > 0)
    {
		if (*ptr++ == '\\')
        {
			return 0;
		}
    }
	return 1;
}

#undef SRE_RECURSIVE

#undef SRE_LITERAL_TEMPLATE
#undef SRE_SEARCH
#undef SRE_MATCH
#undef SRE_INFO
#undef SRE_CHARSET
#undef SRE_COUNT
#undef SRE_AT
#undef SRE_CHAR

#define SRE_CHAR Py_UNICODE
#define SRE_AT sre_uat
#define SRE_COUNT sre_ucount
#define SRE_CHARSET sre_ucharset
#define SRE_INFO sre_uinfo
#define SRE_MATCH sre_umatch
#define SRE_SEARCH sre_usearch
#define SRE_LITERAL_TEMPLATE sre_uliteral_template

LOCAL(int) SRE_AT(SRE_STATE* state, SRE_CHAR* ptr, SRE_CODE at)
{
    int this, that;

    switch (at) 
	{
    case SRE_AT_BEGINNING:
    case SRE_AT_BEGINNING_STRING:
        return ((void*) ptr == state->beginning);

    case SRE_AT_BEGINNING_LINE:
        return ((void*) ptr == state->beginning ||
                SRE_IS_LINEBREAK((int) ptr[-1]));

    case SRE_AT_END:
        return (((void*) (ptr+1) == state->end &&
                 SRE_IS_LINEBREAK((int) ptr[0])) ||
                ((void*) ptr == state->end));

    case SRE_AT_END_LINE:
        return ((void*) ptr == state->end ||
                SRE_IS_LINEBREAK((int) ptr[0]));

    case SRE_AT_END_STRING:
        return ((void*) ptr == state->end);

    case SRE_AT_BOUNDARY:
        if (state->beginning == state->end)
		{
            return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this == that;

    case SRE_AT_LOC_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_LOC_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return this == that;

    case SRE_AT_UNI_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_UNI_NON_BOUNDARY:
        if (state->beginning == state->end)
        {
			return 0;
        }
		that = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return this == that;
    }

    return 0;
}

LOCAL(int) SRE_CHARSET(SRE_CODE* set, SRE_CODE ch)
{
    int ok = 1;

    for (;;) 
	{
        switch (*set++) 
		{
        case SRE_OP_LITERAL:
            if (ch == set[0])
            {
				return ok;
            }
			set++;
            break;

        case SRE_OP_RANGE:
            if (set[0] <= ch && ch <= set[1])
            {
				return ok;
            }
			set += 2;
            break;

        case SRE_OP_CHARSET:
            if (ch < 256 && (set[ch >> 4] & (1 << (ch & 15))))
            {
				return ok;
            }
			set += 16;
            break;

        case SRE_OP_BIGCHARSET:
        {
            int count, block;
            count = *(set++);
            block = ((unsigned char*)set)[ch >> 8];
            set += 128;
            if (set[block*16 + ((ch & 255)>>4)] & (1 << (ch & 15)))
            {
				return ok;
            }
			set += count*16;
            break;
        }

        case SRE_OP_CATEGORY:
            if (sre_category(set[0], (int) ch))
            {
				return ok;
            }
			set += 1;
            break;

        case SRE_OP_NEGATE:
            ok = !ok;
            break;

        case SRE_OP_FAILURE:
            return !ok;

        default:
            return 0;
        }
    }
}

LOCAL(int) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level);

LOCAL(int) SRE_COUNT(SRE_STATE* state, SRE_CODE* pattern, int maxcount, int level)
{
    SRE_CODE chr;
    SRE_CHAR* ptr = state->ptr;
    SRE_CHAR* end = state->end;
    int i;

    if (maxcount < end - ptr && maxcount != 65535)
    {
		end = ptr + maxcount;
	}

    switch (pattern[0]) 
	{
    case SRE_OP_ANY:
        TRACE(("|%p|%p|COUNT ANY\n", pattern, ptr));
        while (ptr < end && !SRE_IS_LINEBREAK(*ptr))
        {
			ptr++;
        }
		break;

    case SRE_OP_ANY_ALL:
        TRACE(("|%p|%p|COUNT ANY_ALL\n", pattern, ptr));
        ptr = end;
        break;

    case SRE_OP_LITERAL:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr == chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_LITERAL_IGNORE:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) == chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_NOT_LITERAL:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr != chr)
        {
			ptr++;
        }
		break;
                
    case SRE_OP_NOT_LITERAL_IGNORE:
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) != chr)
        {
			ptr++;
        }
		break;

    case SRE_OP_IN:
        TRACE(("|%p|%p|COUNT IN\n", pattern, ptr));
        while (ptr < end && SRE_CHARSET(pattern + 2, *ptr))
        {
			ptr++;
        }
		break;

    default:
        TRACE(("|%p|%p|COUNT SUBPATTERN\n", pattern, ptr));
        while ((SRE_CHAR*) state->ptr < end) 
		{
            i = SRE_MATCH(state, pattern, level);
            if (i < 0)
            {
				return i;
            }
			if (!i)
            {
				break;
			}
        }
        TRACE(("|%p|%p|COUNT %d\n", pattern, ptr,
               (SRE_CHAR*) state->ptr - ptr));
        return (SRE_CHAR*) state->ptr - ptr;
    }

    TRACE(("|%p|%p|COUNT %d\n", pattern, ptr, ptr - (SRE_CHAR*) state->ptr));
    return ptr - (SRE_CHAR*) state->ptr;
}

LOCAL(int) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level)
{
    SRE_CHAR* end = state->end;
    SRE_CHAR* ptr = state->ptr;
    int i, count;
    SRE_REPEAT* rp;
    int lastmark;
    SRE_CODE chr;

    SRE_REPEAT rep; 

    TRACE(("|%p|%p|ENTER %d\n", pattern, ptr, level));

    if (level % 10 == 0 && PyOS_CheckStack())
    {
		return SRE_ERROR_RECURSION_LIMIT;
	}

    if (pattern[0] == SRE_OP_INFO) 
	{
        if (pattern[3] && (end - ptr) < pattern[3]) 
		{
            TRACE(("reject (got %d chars, need %d)\n",
                   (end - ptr), pattern[3]));
            return 0;
        }
        pattern += pattern[1] + 1;
    }

    for (;;) 
	{
        switch (*pattern++) 
		{
        case SRE_OP_FAILURE:
            TRACE(("|%p|%p|FAILURE\n", pattern, ptr));
            return 0;

        case SRE_OP_SUCCESS:
            TRACE(("|%p|%p|SUCCESS\n", pattern, ptr));
            state->ptr = ptr;
            return 1;

        case SRE_OP_AT:
            TRACE(("|%p|%p|AT %d\n", pattern, ptr, *pattern));
            if (!SRE_AT(state, ptr, *pattern))
            {
				return 0;
            }
			pattern++;
            break;

        case SRE_OP_CATEGORY:
            TRACE(("|%p|%p|CATEGORY %d\n", pattern, ptr, *pattern));
            if (ptr >= end || !sre_category(pattern[0], ptr[0]))
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_LITERAL:
            TRACE(("|%p|%p|LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] != pattern[0])
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL:
            TRACE(("|%p|%p|NOT_LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] == pattern[0])
            {
				return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_ANY:
            TRACE(("|%p|%p|ANY\n", pattern, ptr));
            if (ptr >= end || SRE_IS_LINEBREAK(ptr[0]))
            {
				return 0;
            }
			ptr++;
            break;

        case SRE_OP_ANY_ALL:
            TRACE(("|%p|%p|ANY_ALL\n", pattern, ptr));
            if (ptr >= end)
            {
				return 0;
            }
			ptr++;
            break;

        case SRE_OP_IN:
            TRACE(("|%p|%p|IN\n", pattern, ptr));
            if (ptr >= end || !SRE_CHARSET(pattern + 1, *ptr))
            {
				return 0;
            }
			pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_GROUPREF:
            TRACE(("|%p|%p|GROUPREF %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                {
					return 0;
                }
				while (p < e) 
				{
                    if (ptr >= end || *ptr != *p)
                    {
						return 0;
                    }
					p++; 
					ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_GROUPREF_IGNORE:
            TRACE(("|%p|%p|GROUPREF_IGNORE %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                {
					return 0;
                }
				while (p < e) 
				{
                    if (ptr >= end ||
                        state->lower(*ptr) != state->lower(*p))
                    {
						return 0;
                    }
					p++; 
					ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_LITERAL_IGNORE:
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n", pattern, ptr, pattern[0]));
            if (ptr >= end ||
                state->lower(*ptr) != state->lower(*pattern))
			{
                return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n", pattern, ptr, *pattern));
            if (ptr >= end ||
                state->lower(*ptr) == state->lower(*pattern))
			{
                return 0;
            }
			pattern++;
            ptr++;
            break;

        case SRE_OP_IN_IGNORE:
            TRACE(("|%p|%p|IN_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE_CHARSET(pattern + 1, (SRE_CODE) state->lower(*ptr)))
			{
                return 0;
            }
			pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_MARK:
            TRACE(("|%p|%p|MARK %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            if (i & 1)
			{
                state->lastindex = i/2 + 1;
            }
			if (i > state->lastmark)
            {
				state->lastmark = i;
            }
			state->mark[i] = ptr;
            pattern++;
            break;

        case SRE_OP_JUMP:
        case SRE_OP_INFO:
            TRACE(("|%p|%p|JUMP %d\n", pattern, ptr, pattern[0]));
            pattern += pattern[0];
            break;

        case SRE_OP_ASSERT:
            TRACE(("|%p|%p|ASSERT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr < state->beginning)
            {
				return 0;
            }
			i = SRE_MATCH(state, pattern + 2, level + 1);
            if (i <= 0)
            {
				return i;
            }
			pattern += pattern[0];
            break;

        case SRE_OP_ASSERT_NOT:
            TRACE(("|%p|%p|ASSERT_NOT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr >= state->beginning) 
			{
                i = SRE_MATCH(state, pattern + 2, level + 1);
                if (i < 0)
                {
					return i;
                }
				if (i)
                {
					return 0;
				}
            }
            pattern += pattern[0];
            break;

        case SRE_OP_BRANCH:
            TRACE(("|%p|%p|BRANCH\n", pattern, ptr));
            lastmark = state->lastmark;
            for (; pattern[0]; pattern += pattern[0]) 
			{
                if (pattern[1] == SRE_OP_LITERAL &&
                    (ptr >= end || (SRE_CODE) *ptr != pattern[2]))
				{
                    continue;
                }
				if (pattern[1] == SRE_OP_IN &&
                    (ptr >= end || !SRE_CHARSET(pattern + 3, (SRE_CODE) *ptr)))
				{
                    continue;
                }
				state->ptr = ptr;
                i = SRE_MATCH(state, pattern + 1, level + 1);
                if (i)
				{
                    return i;
                }
				if (state->lastmark > lastmark) 
				{
                    memset(
                        state->mark + lastmark + 1, 0,
                        (state->lastmark - lastmark) * sizeof(void*)
                        );
                    state->lastmark = lastmark;
                }
            }
            return 0;

        case SRE_OP_REPEAT_ONE:
            TRACE(("|%p|%p|REPEAT_ONE %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            if (ptr + pattern[1] > end)
            {
				return 0;
			}

            state->ptr = ptr;

            count = SRE_COUNT(state, pattern + 3, pattern[2], level + 1);
            if (count < 0)
			{
                return count;
			}

            ptr += count;

            if (count < (int) pattern[1])
            {
				return 0;
			}

            if (pattern[pattern[0]] == SRE_OP_SUCCESS) 
			{
                state->ptr = ptr;
                return 1;

            } 
			else if (pattern[pattern[0]] == SRE_OP_LITERAL) 
			{
                chr = pattern[pattern[0]+1];
                for (;;) 
				{
                    while (count >= (int) pattern[1] &&
                           (ptr >= end || *ptr != chr)) 
					{
                        ptr--;
                        count--;
                    }
                    if (count < (int) pattern[1])
                    {
						break;
                    }
					state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
					{
                        return i;
                    }
					ptr--;
                    count--;
                }
            } 
			else 
			{
                lastmark = state->lastmark;
                while (count >= (int) pattern[1]) 
				{
                    state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
					{
                        return i;
                    }
					ptr--;
                    count--;
                    if (state->lastmark > lastmark) 
					{
                        memset(
                            state->mark + lastmark + 1, 0,
                            (state->lastmark - lastmark) * sizeof(void*)
                            );
                        state->lastmark = lastmark;
                    }
                }
            }
            return 0;

        case SRE_OP_REPEAT:
            TRACE(("|%p|%p|REPEAT %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            rep.count = -1;
            rep.pattern = pattern;

            rep.prev = state->repeat;
            state->repeat = &rep;

            state->ptr = ptr;
            i = SRE_MATCH(state, pattern + pattern[0], level + 1);

            state->repeat = rep.prev;

            return i;

        case SRE_OP_MAX_UNTIL:
            rp = state->repeat;
            if (!rp)
			{
                return SRE_ERROR_STATE;
			}

            state->ptr = ptr;

            count = rp->count + 1;

            TRACE(("|%p|%p|MAX_UNTIL %d\n", pattern, ptr, count));

            if (count < rp->pattern[1]) 
			{
                rp->count = count;
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				rp->count = count - 1;
                state->ptr = ptr;
                return 0;
            }

            if (count < rp->pattern[2] || rp->pattern[2] == 65535) 
			{
                rp->count = count;
                lastmark = state->lastmark;
                i = mark_save(state, 0, lastmark);
                if (i < 0)
                {
					return i;
                }
				i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				i = mark_restore(state, 0, lastmark);
                state->lastmark = lastmark;
                if (i < 0)
                {
					return i;
                }
				rp->count = count - 1;
                state->ptr = ptr;
            }
            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i)
            {
				return i;
            }
			state->repeat = rp;
            state->ptr = ptr;
            return 0;

        case SRE_OP_MIN_UNTIL:
            rp = state->repeat;
            if (!rp)
			{
                return SRE_ERROR_STATE;
			}

            count = rp->count + 1;

            TRACE(("|%p|%p|MIN_UNTIL %d %p\n", pattern, ptr, count,
                   rp->pattern));

            state->ptr = ptr;

            if (count < rp->pattern[1]) 
			{
                rp->count = count;
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                {
					return i;
                }
				rp->count = count-1;
                state->ptr = ptr;
                return 0;
            }

            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i)
			{
                return i;
			}

            state->ptr = ptr;
            state->repeat = rp;

            if (count >= rp->pattern[2] && rp->pattern[2] != 65535)
            {
				return 0;
			}

            rp->count = count;
            i = SRE_MATCH(state, rp->pattern + 3, level + 1);
            if (i)
            {
				return i;
            }
			rp->count = count - 1;
            state->ptr = ptr;
            return 0;

        default:
            TRACE(("|%p|%p|UNKNOWN %d\n", pattern, ptr, pattern[-1]));
            return SRE_ERROR_ILLEGAL;
        }
    }
}

LOCAL(int) SRE_SEARCH(SRE_STATE* state, SRE_CODE* pattern)
{
    SRE_CHAR* ptr = state->start;
    SRE_CHAR* end = state->end;
    int status = 0;
    int prefix_len = 0;
    int prefix_skip = 0;
    SRE_CODE* prefix = NULL;
    SRE_CODE* charset = NULL;
    SRE_CODE* overlap = NULL;
    int flags = 0;

    if (pattern[0] == SRE_OP_INFO) 
	{
        flags = pattern[2];

        if (pattern[3] > 0) 
		{
            end -= pattern[3]-1;
            if (end <= ptr)
            {
				end = ptr + 1;
			}
        }

        if (flags & SRE_INFO_PREFIX) 
		{
            prefix_len = pattern[5];
            prefix_skip = pattern[6];
            prefix = pattern + 7;
            overlap = prefix + prefix_len - 1;
        } 
		else if (flags & SRE_INFO_CHARSET)
		{
            charset = pattern + 5;
		}

        pattern += 1 + pattern[1];
    }

    TRACE(("prefix = %p %d %d\n", prefix, prefix_len, prefix_skip));
    TRACE(("charset = %p\n", charset));

    if (prefix_len > 1) 
	{
        int i = 0;
        end = state->end;
        while (ptr < end) 
		{
            for (;;) 
			{
                if ((SRE_CODE) ptr[0] != prefix[i]) 
				{
                    if (!i)
                    {
						break;
                    }
					else
                    {
						i = overlap[i];
					}
                } 
				else 
				{
                    if (++i == prefix_len) 
					{
                        TRACE(("|%p|%p|SEARCH SCAN\n", pattern, ptr));
                        state->start = ptr + 1 - prefix_len;
                        state->ptr = ptr + 1 - prefix_len + prefix_skip;
                        if (flags & SRE_INFO_LITERAL)
                        {
							return 1;
                        }
						status = SRE_MATCH(state, pattern + 2*prefix_skip, 1);
                        if (status != 0)
						{
                            return status;
                        }
						i = overlap[i];
                    }
                    break;
                }
                
            }
            ptr++;
        }
        return 0;
    }

    if (pattern[0] == SRE_OP_LITERAL) 
	{
        SRE_CODE chr = pattern[1];
        end = state->end;
        for (;;) 
		{
            while (ptr < end && (SRE_CODE) ptr[0] != chr)
            {
				ptr++;
            }
			if (ptr == end)
            {
				return 0;
            }
			TRACE(("|%p|%p|SEARCH LITERAL\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ++ptr;
            if (flags & SRE_INFO_LITERAL)
            {
				return 1;
            }
			status = SRE_MATCH(state, pattern + 2, 1);
            if (status != 0)
			{
                break;
			}
        }
    } 
	else if (charset) 
	{
        end = state->end;
        for (;;) 
		{
            while (ptr < end && !SRE_CHARSET(charset, ptr[0]))
            {
				ptr++;
            }
			if (ptr == end)
            {
				return 0;
            }
			TRACE(("|%p|%p|SEARCH CHARSET\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
            {
				break;
			}
            ptr++;
        }
    } 
	else
    {
		while (ptr <= end) 
		{
            TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
            state->start = state->ptr = ptr++;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
            {
				break;
			}
        }
	}
    return status;
}
    
LOCAL(int) SRE_LITERAL_TEMPLATE(SRE_CHAR* ptr, int len)
{
    while (len-- > 0)
    {
		if (*ptr++ == '\\')
        {
			return 0;
		}
    }
	return 1;
}

staticforward PyTypeObject Pattern_Type;
staticforward PyTypeObject Match_Type;
staticforward PyTypeObject Scanner_Type;

static PyObject *_compile(PyObject* self_, PyObject* args)
{
    PatternObject* self;
    int i, n;

    PyObject* pattern;
    int flags = 0;
    PyObject* code;
    int groups = 0;
    PyObject* groupindex = NULL;
    PyObject* indexgroup = NULL;
    if (!PyArg_ParseTuple(args, "OiO!|iOO", &pattern, &flags,
                          &PyList_Type, &code, &groups,
                          &groupindex, &indexgroup))
	{
        return NULL;
	}

    n = PyList_GET_SIZE(code);

    self = PyObject_NEW_VAR(PatternObject, &Pattern_Type, n);
    if (!self)
    {
		return NULL;
	}

    self->codesize = n;

    for (i = 0; i < n; i++) 
	{
        PyObject *o = PyList_GET_ITEM(code, i);
        self->code[i] = (SRE_CODE) PyInt_AsLong(o);
    }

    if (PyErr_Occurred()) 
	{
        PyObject_DEL(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = pattern;

    self->flags = flags;

    self->groups = groups;

    Py_XINCREF(groupindex);
    self->groupindex = groupindex;

    Py_XINCREF(indexgroup);
    self->indexgroup = indexgroup;

    return (PyObject*) self;
}

static PyObject *sre_codesize(PyObject* self, PyObject* args)
{
    return Py_BuildValue("i", sizeof(SRE_CODE));
}

static PyObject *sre_getlower(PyObject* self, PyObject* args)
{
    int character, flags;
    if (!PyArg_ParseTuple(args, "ii", &character, &flags))
    {
		return NULL;
    }
	if (flags & SRE_FLAG_LOCALE)
    {
		return Py_BuildValue("i", sre_lower_locale(character));
    }
	if (flags & SRE_FLAG_UNICODE)
	{
        return Py_BuildValue("i", sre_lower_unicode(character));
	}
    return Py_BuildValue("i", sre_lower(character));
}

LOCAL(void) state_reset(SRE_STATE* state)
{
    int i;

    state->lastmark = 0;

    for (i = 0; i < SRE_MARK_SIZE; i++)
    {
		state->mark[i] = NULL;
	}

    state->lastindex = -1;

    state->repeat = NULL;

    mark_fini(state);
}

static void *getstring(PyObject* string, int* p_length, int* p_charsize)
{
    PyBufferProcs *buffer;
    int size, bytes, charsize;
    void* ptr;

    if (PyUnicode_Check(string)) 
	{
        ptr = (void*) PyUnicode_AS_DATA(string);
        bytes = PyUnicode_GET_DATA_SIZE(string);
        size = PyUnicode_GET_SIZE(string);
        charsize = sizeof(Py_UNICODE);
    } 
	else 
	{
		buffer = string->ob_type->tp_as_buffer;
		if (!buffer || !buffer->bf_getreadbuffer || !buffer->bf_getsegcount ||
			buffer->bf_getsegcount(string, NULL) != 1) 
		{
			PyErr_SetString(PyExc_TypeError, "expected string or buffer");
			return NULL;
		}

		bytes = buffer->bf_getreadbuffer(string, 0, &ptr);
		if (bytes < 0) 
		{
			PyErr_SetString(PyExc_TypeError, "buffer has negative size");
			return NULL;
		}

		size = PyObject_Size(string);

		if (PyString_Check(string) || bytes == size)
		{
			charsize = 1;
		}
		else if (bytes == (int) (size * sizeof(Py_UNICODE)))
		{
			charsize = sizeof(Py_UNICODE);
		}
		else 
		{
			PyErr_SetString(PyExc_TypeError, "buffer size mismatch");
			return NULL;
		}
    }

    *p_length = size;
    *p_charsize = charsize;

    return ptr;
}

LOCAL(PyObject*) state_init(SRE_STATE* state, PatternObject* pattern, PyObject* string,
           int start, int end)
{
    int length;
    int charsize;
    void* ptr;

    memset(state, 0, sizeof(SRE_STATE));

    state->lastindex = -1;

    ptr = getstring(string, &length, &charsize);
    if (!ptr)
    {
		return NULL;
	}

    if (start < 0)
    {
		start = 0;
    }
	else if (start > length)
    {
		start = length;
	}

    if (end < 0)
    {
		end = 0;
    }
	else if (end > length)
    {
		end = length;
	}

    state->charsize = charsize;

    state->beginning = ptr;

    state->start = (void*) ((char*) ptr + start * state->charsize);
    state->end = (void*) ((char*) ptr + end * state->charsize);

    Py_INCREF(string);
    state->string = string;
    state->pos = start;
    state->endpos = end;

    if (pattern->flags & SRE_FLAG_LOCALE)
	{
        state->lower = sre_lower_locale;
    }
	else if (pattern->flags & SRE_FLAG_UNICODE)
	{
        state->lower = sre_lower_unicode;
    }
	else
	{
        state->lower = sre_lower;
	}

    return string;
}

LOCAL(void) state_fini(SRE_STATE* state)
{
    Py_XDECREF(state->string);
    mark_fini(state);
}

#define STATE_OFFSET(state, member)\
    (((char*)(member) - (char*)(state)->beginning) / (state)->charsize)

LOCAL(PyObject*) state_getslice(SRE_STATE* state, int index, PyObject* string, int empty)
{
    int i, j;

    index = (index - 1) * 2;

    if (string == Py_None || !state->mark[index] || !state->mark[index+1]) 
	{
        if (empty)
        {
			i = j = 0;
        }
		else 
		{
            Py_INCREF(Py_None);
            return Py_None;
        }
    } 
	else 
	{
        i = STATE_OFFSET(state, state->mark[index]);
        j = STATE_OFFSET(state, state->mark[index+1]);
    }

    return PySequence_GetSlice(string, i, j);
}

static void pattern_error(int status)
{
    switch (status) 
	{
    case SRE_ERROR_RECURSION_LIMIT:
        PyErr_SetString(
            PyExc_RuntimeError,
            "maximum recursion limit exceeded"
            );
        break;
    
	case SRE_ERROR_MEMORY:
        PyErr_NoMemory();
        break;
    
	default:
        PyErr_SetString(
            PyExc_RuntimeError,
            "internal error in regular expression engine"
            );
    }
}

static PyObject *pattern_new_match(PatternObject* pattern, SRE_STATE* state, int status)
{
    MatchObject* match;
    int i, j;
    char* base;
    int n;

    if (status > 0) 
	{
	    match = PyObject_NEW_VAR(MatchObject, &Match_Type,
                                 2*(pattern->groups+1));
        if (!match)
        {
			return NULL;
		}

        Py_INCREF(pattern);
        match->pattern = pattern;

        Py_INCREF(state->string);
        match->string = state->string;

        match->regs = NULL;
        match->groups = pattern->groups+1;

        base = (char*) state->beginning;
        n = state->charsize;

        match->mark[0] = ((char*) state->start - base) / n;
        match->mark[1] = ((char*) state->ptr - base) / n;

        for (i = j = 0; i < pattern->groups; i++, j += 2)
        {
			if (j+1 <= state->lastmark && state->mark[j] && state->mark[j+1]) 
			{
                match->mark[j+2] = ((char*) state->mark[j] - base) / n;
                match->mark[j+3] = ((char*) state->mark[j+1] - base) / n;
            } 
			else
            {
				match->mark[j+2] = match->mark[j+3] = -1;
			}
		}

        match->pos = state->pos;
        match->endpos = state->endpos;

        match->lastindex = state->lastindex;

        return (PyObject*) match;
    } 
	else if (status == 0) 
	{
        Py_INCREF(Py_None);
        return Py_None;
    }
    pattern_error(status);
    return NULL;
}

static PyObject *pattern_scanner(PatternObject* pattern, PyObject* args)
{
    ScannerObject* self;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    if (!PyArg_ParseTuple(args, "O|ii:scanner", &string, &start, &end))
    {
		return NULL;
	}

    self = PyObject_NEW(ScannerObject, &Scanner_Type);
    if (!self)
    {
		return NULL;
	}

    string = state_init(&self->state, pattern, string, start, end);
    if (!string) 
	{
        PyObject_DEL(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = (PyObject*) pattern;

    return (PyObject*) self;
}

static void pattern_dealloc(PatternObject* self)
{
    Py_XDECREF(self->pattern);
    Py_XDECREF(self->groupindex);
    Py_XDECREF(self->indexgroup);
    PyObject_DEL(self);
}

static PyObject *pattern_match(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:match", kwlist,
                                     &string, &start, &end))
	{
        return NULL;
	}

    string = state_init(&state, self, string, start, end);
    if (!string)
    {
		return NULL;
	}

    state.ptr = state.start;

    TRACE(("|%p|%p|MATCH\n", PatternObject_GetCode(self), state.ptr));

    if (state.charsize == 1) 
	{
        status = sre_match(&state, PatternObject_GetCode(self), 1);
    } 
	else 
	{
        status = sre_umatch(&state, PatternObject_GetCode(self), 1);
    }

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject *pattern_search(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:search", kwlist,
                                     &string, &start, &end))
	{
        return NULL;
	}

    string = state_init(&state, self, string, start, end);
    if (!string)
    {
		return NULL;
	}

    TRACE(("|%p|%p|SEARCH\n", PatternObject_GetCode(self), state.ptr));

    if (state.charsize == 1) 
	{
        status = sre_search(&state, PatternObject_GetCode(self));
    } 
	else 
	{
        status = sre_usearch(&state, PatternObject_GetCode(self));
    }

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject *call(char* module, char* function, PyObject* args)
{
    PyObject* name;
    PyObject* mod;
    PyObject* func;
    PyObject* result;

    if (!args)
	{
        return NULL;
    }
	name = PyString_FromString(module);
    if (!name)
    {
		return NULL;
    }
	mod = PyImport_Import(name);
    Py_DECREF(name);
    if (!mod)
    {
		return NULL;
    }
	func = PyObject_GetAttrString(mod, function);
    Py_DECREF(mod);
    if (!func)
    {
		return NULL;
    }
	result = PyObject_CallObject(func, args);
    Py_DECREF(func);
    Py_DECREF(args);
    return result;
}

static PyObject *join_list(PyObject* list, PyObject* pattern)
{
    PyObject* joiner;
    PyObject* function;
    PyObject* args;
    PyObject* result;

    switch (PyList_GET_SIZE(list)) 
	{
    case 0:
        Py_DECREF(list);
        return PyString_FromString("");
    
	case 1:
        result = PyList_GET_ITEM(list, 0);
        Py_INCREF(result);
        Py_DECREF(list);
        return result;
    }
    joiner = PySequence_GetSlice(pattern, 0, 0);
    if (!joiner)
	{
		return NULL;
	}

    function = PyObject_GetAttrString(joiner, "join");
    if (!function) 
	{
        Py_DECREF(joiner);
        return NULL;
    }
    args = PyTuple_New(1);
    if (!args) 
	{
        Py_DECREF(function);
        Py_DECREF(joiner);
        return NULL;
    }
    PyTuple_SET_ITEM(args, 0, list);
    result = PyObject_CallObject(function, args);
    Py_DECREF(args);
    Py_DECREF(function);
    Py_DECREF(joiner);

    return result;
}

static PyObject *pattern_findall(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    int status;
    int i, b, e;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "source", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:findall", kwlist,
                                     &string, &start, &end))
	{
		return NULL;
	}

    string = state_init(&state, self, string, start, end);
    if (!string)
	{
        return NULL;
	}

    list = PyList_New(0);
    if (!list) 
	{
        state_fini(&state);
        return NULL;
    }

    while (state.start <= state.end) 
	{
        PyObject* item;
        
        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) 
		{
            status = sre_search(&state, PatternObject_GetCode(self));
        } 
		else 
		{
            status = sre_usearch(&state, PatternObject_GetCode(self));
        }

        if (status <= 0) 
		{
            if (status == 0)
			{
                break;
            }
			pattern_error(status);
            goto error;
        }
        
        switch (self->groups) 
		{
        case 0:
            b = STATE_OFFSET(&state, state.start);
            e = STATE_OFFSET(&state, state.ptr);
            item = PySequence_GetSlice(string, b, e);
            if (!item)
            {
				goto error;
            }
			break;
        
		case 1:
            item = state_getslice(&state, 1, string, 1);
            if (!item)
            {
				goto error;
            }
			break;
        
		default:
            item = PyTuple_New(self->groups);
            if (!item)
			{
                goto error;
            }
			for (i = 0; i < self->groups; i++) 
			{
                PyObject* o = state_getslice(&state, i+1, string, 1);
                if (!o) 
				{
                    Py_DECREF(item);
                    goto error;
                }
                PyTuple_SET_ITEM(item, i, o);
            }
            break;
        }

        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
		{
            goto error;
		}

        if (state.ptr == state.start)
        {
			state.start = (void*) ((char*) state.ptr + state.charsize);
        }
		else
        {
			state.start = state.ptr;
		}
    }

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;
}

static PyObject *pattern_finditer(PatternObject* pattern, PyObject* args)
{
    PyObject* scanner;
    PyObject* search;
    PyObject* iterator;

    scanner = pattern_scanner(pattern, args);
    if (!scanner)
	{
        return NULL;
	}

    search = PyObject_GetAttrString(scanner, "search");
    Py_DECREF(scanner);
    if (!search)
    {
		return NULL;
	}

    iterator = PyCallIter_New(search, Py_None);
    Py_DECREF(search);

    return iterator;
}

static PyObject *pattern_split(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    int status;
    int n;
    int i;
    void* last;

    PyObject* string;
    int maxsplit = 0;
    static char* kwlist[] = { "source", "maxsplit", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|i:split", kwlist,
                                     &string, &maxsplit))
    {
		return NULL;
	}

    string = state_init(&state, self, string, 0, INT_MAX);
    if (!string)
    {
		return NULL;
	}

    list = PyList_New(0);
    if (!list) {
        state_fini(&state);
        return NULL;
    }

    n = 0;
    last = state.start;

    while (!maxsplit || n < maxsplit) 
	{
        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) 
		{
            status = sre_search(&state, PatternObject_GetCode(self));
        } 
		else 
		{
            status = sre_usearch(&state, PatternObject_GetCode(self));
        }

        if (status <= 0) 
		{
            if (status == 0)
            {
				break;
            }
			pattern_error(status);
            goto error;
        }
        
        if (state.start == state.ptr) 
		{
            if (last == state.end)
            {
				break;
            }
			state.start = (void*) ((char*) state.ptr + state.charsize);
            continue;
        }

        item = PySequence_GetSlice(
            string, STATE_OFFSET(&state, last),
            STATE_OFFSET(&state, state.start)
            );
        if (!item)
        {
			goto error;
        }
		status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
		{
			goto error;
		}

        for (i = 0; i < self->groups; i++) 
		{
            item = state_getslice(&state, i+1, string, 0);
            if (!item)
            {
				goto error;
            }
			status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
            {
				goto error;
			}
        }

        n = n + 1;

        last = state.start = state.ptr;

    }

    item = PySequence_GetSlice(
        string, STATE_OFFSET(&state, last), state.endpos
        );
    if (!item)
    {
		goto error;
    }
	status = PyList_Append(list, item);
    Py_DECREF(item);
    if (status < 0)
    {
		goto error;
	}

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;
}

static PyObject *pattern_subx(PatternObject* self, PyObject* template, PyObject* string,
             int count, int subn)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    PyObject* filter;
    PyObject* args;
    PyObject* match;
    void* ptr;
    int status;
    int n;
    int i, b, e;
    int filter_is_callable;

    if (PyCallable_Check(template)) 
	{
        filter = template;
        Py_INCREF(filter);
        filter_is_callable = 1;
    } 
	else 
	{
        int literal;
        ptr = getstring(template, &n, &b);
        if (ptr)
		{
            if (b == 1) 
			{
                literal = sre_literal_template(ptr, n);
            } 
			else 
			{
                literal = sre_uliteral_template(ptr, n);
            }
        } 
		else 
		{
            PyErr_Clear();
            literal = 0;
        }
        if (literal) 
		{
            filter = template;
            Py_INCREF(filter);
            filter_is_callable = 0;
        } 
		else 
		{
            filter = call(
                SRE_MODULE, "_subx",
                Py_BuildValue("OO", self, template)
                );
            if (!filter)
			{
                return NULL;
            }
			filter_is_callable = PyCallable_Check(filter);
        }
    }

    string = state_init(&state, self, string, 0, INT_MAX);
    if (!string) 
	{
        Py_DECREF(filter);
        return NULL;
    }

    list = PyList_New(0);
    if (!list) 
	{
        Py_DECREF(filter);
        state_fini(&state);
        return NULL;
    }

    n = i = 0;

    while (!count || n < count) 
	{
        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) 
		{
            status = sre_search(&state, PatternObject_GetCode(self));
        } 
		else 
		{
            status = sre_usearch(&state, PatternObject_GetCode(self));
        }

        if (status <= 0) 
		{
            if (status == 0)
            {
				break;
            }
			pattern_error(status);
            goto error;
        }
        
        b = STATE_OFFSET(&state, state.start);
        e = STATE_OFFSET(&state, state.ptr);

        if (i < b) 
		{
            item = PySequence_GetSlice(string, i, b);
            if (!item)
            {
				goto error;
            }
			status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
            {
				goto error;
			}
        } 
		else if (i == b && i == e && n > 0)
        {
			goto next;
		}

        if (filter_is_callable) 
		{
            match = pattern_new_match(self, &state, 1);
            if (!match)
            {
				goto error;
            }
			args = Py_BuildValue("(O)", match);
            if (!args) 
			{
                Py_DECREF(match);
                goto error;
            }
            item = PyObject_CallObject(filter, args);
            Py_DECREF(args);
            Py_DECREF(match);
            if (!item)
            {
				goto error;
			}
        } 
		else 
		{
            item = filter;
            Py_INCREF(item);
        }

        if (item != Py_None) 
		{
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
            {
				goto error;
			}
        }
        i = e;
        n = n + 1;
next:
        if (state.ptr == state.start)
        {
			state.start = (void*) ((char*) state.ptr + state.charsize);
        }
		else
        {
			state.start = state.ptr;
		}
    }

    if (i < state.endpos) 
	{
        item = PySequence_GetSlice(string, i, state.endpos);
        if (!item)
        {
			goto error;
        }
		status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
        {
			goto error;
		}
    }

    state_fini(&state);

    Py_DECREF(filter);

    item = join_list(list, self->pattern);

    if (!item)
    {
		return NULL;
	}

    if (subn)
    {
		return Py_BuildValue("Ni", item, n);
	}

    return item;

error:
    Py_DECREF(list);
    state_fini(&state);
    Py_DECREF(filter);
    return NULL;
}

static PyObject *pattern_sub(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* template;
    PyObject* string;
    int count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|i:sub", kwlist,
                                     &template, &string, &count))
    {
		return NULL;
	}

    return pattern_subx(self, template, string, count, 0);
}

static PyObject *pattern_subn(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* template;
    PyObject* string;
    int count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|i:subn", kwlist,
                                     &template, &string, &count))
	{
        return NULL;
	}

    return pattern_subx(self, template, string, count, 1);
}

static PyObject *pattern_copy(PatternObject* self, PyObject* args)
{
    PyErr_SetString(PyExc_TypeError, "cannot copy this pattern object");
    return NULL;
}

static PyObject *pattern_deepcopy(PatternObject* self, PyObject* args)
{
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this pattern object");
    return NULL;
}

static PyMethodDef pattern_methods[] = {
    {"match", (PyCFunction) pattern_match, METH_VARARGS|METH_KEYWORDS},
    {"search", (PyCFunction) pattern_search, METH_VARARGS|METH_KEYWORDS},
    {"sub", (PyCFunction) pattern_sub, METH_VARARGS|METH_KEYWORDS},
    {"subn", (PyCFunction) pattern_subn, METH_VARARGS|METH_KEYWORDS},
    {"split", (PyCFunction) pattern_split, METH_VARARGS|METH_KEYWORDS},
    {"findall", (PyCFunction) pattern_findall, METH_VARARGS|METH_KEYWORDS},
    {"finditer", (PyCFunction) pattern_finditer, METH_VARARGS},
    {"scanner", (PyCFunction) pattern_scanner, METH_VARARGS},
    {"__copy__", (PyCFunction) pattern_copy, METH_VARARGS},
    {"__deepcopy__", (PyCFunction) pattern_deepcopy, METH_VARARGS},
    {NULL, NULL}
};

static PyObject *pattern_getattr(PatternObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(pattern_methods, (PyObject*) self, name);

    if (res)
	{
        return res;
	}

    PyErr_Clear();

    if (!strcmp(name, "pattern")) 
	{
        Py_INCREF(self->pattern);
        return self->pattern;
    }

    if (!strcmp(name, "flags"))
	{
        return Py_BuildValue("i", self->flags);
	}

    if (!strcmp(name, "groups"))
    {
		return Py_BuildValue("i", self->groups);
	}

    if (!strcmp(name, "groupindex") && self->groupindex) 
	{
        Py_INCREF(self->groupindex);
        return self->groupindex;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Pattern_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "_" SRE_MODULE ".SRE_Pattern",
    sizeof(PatternObject), sizeof(SRE_CODE),
    (destructor)pattern_dealloc,
    0,
    (getattrfunc)pattern_getattr
};

static void match_dealloc(MatchObject* self)
{
    Py_XDECREF(self->regs);
    Py_XDECREF(self->string);
    Py_DECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject *match_getslice_by_index(MatchObject* self, int index, PyObject* def)
{
    if (index < 0 || index >= self->groups) 
	{
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    index *= 2;

    if (self->string == Py_None || self->mark[index] < 0) 
	{
        Py_INCREF(def);
        return def;
    }

    return PySequence_GetSlice(
        self->string, self->mark[index], self->mark[index+1]
        );
}

static int match_getindex(MatchObject* self, PyObject* index)
{
    int i;

    if (PyInt_Check(index))
    {
		return (int) PyInt_AS_LONG(index);
	}

    i = -1;

    if (self->pattern->groupindex) 
	{
        index = PyObject_GetItem(self->pattern->groupindex, index);
        if (index) 
		{
            if (PyInt_Check(index))
            {
				i = (int) PyInt_AS_LONG(index);
            }
			Py_DECREF(index);
        } 
		else
        {
			PyErr_Clear();
		}
    }

    return i;
}

static PyObject *match_getslice(MatchObject* self, PyObject* index, PyObject* def)
{
    return match_getslice_by_index(self, match_getindex(self, index), def);
}

static PyObject *match_expand(MatchObject* self, PyObject* args)
{
    PyObject* template;
    if (!PyArg_ParseTuple(args, "O:expand", &template))
    {
		return NULL;
	}

    return call(
        SRE_MODULE, "_expand",
        Py_BuildValue("OOO", self->pattern, self, template)
        );
}

static PyObject *match_group(MatchObject* self, PyObject* args)
{
    PyObject* result;
    int i, size;

    size = PyTuple_GET_SIZE(args);

    switch (size) 
	{
    case 0:
        result = match_getslice(self, Py_False, Py_None);
        break;
    
	case 1:
        result = match_getslice(self, PyTuple_GET_ITEM(args, 0), Py_None);
        break;
    
	default:
        result = PyTuple_New(size);
        if (!result)
        {
			return NULL;
        }
		for (i = 0; i < size; i++) 
		{
            PyObject* item = match_getslice(
                self, PyTuple_GET_ITEM(args, i), Py_None
                );
            if (!item) 
			{
                Py_DECREF(result);
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, item);
        }
        break;
    }
    return result;
}

static PyObject *match_groups(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    int index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groups", kwlist, &def))
    {
		return NULL;
	}

    result = PyTuple_New(self->groups-1);
    if (!result)
    {
		return NULL;
	}

    for (index = 1; index < self->groups; index++) 
	{
        PyObject* item;
        item = match_getslice_by_index(self, index, def);
        if (!item) 
		{
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, index-1, item);
    }

    return result;
}

static PyObject *match_groupdict(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    PyObject* keys;
    int index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groupdict", kwlist, &def))
	{
        return NULL;
	}

    result = PyDict_New();
    if (!result || !self->pattern->groupindex)
    {
		return result;
	}

    keys = PyMapping_Keys(self->pattern->groupindex);
    if (!keys)
    {
		goto failed;
	}

    for (index = 0; index < PyList_GET_SIZE(keys); index++) 
	{
        int status;
        PyObject* key;
        PyObject* value;
        key = PyList_GET_ITEM(keys, index);
        if (!key)
        {
			goto failed;
        }
		value = match_getslice(self, key, def);
        if (!value) 
		{
            Py_DECREF(key);
            goto failed;
        }
        status = PyDict_SetItem(result, key, value);
        Py_DECREF(value);
        if (status < 0)
		{
            goto failed;
		}
    }

    Py_DECREF(keys);

    return result;

failed:
    Py_DECREF(keys);
    Py_DECREF(result);
    return NULL;
}

static PyObject *match_start(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False;
    if (!PyArg_ParseTuple(args, "|O:start", &index_))
    {
		return NULL;
	}

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) 
	{
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }
    return Py_BuildValue("i", self->mark[index*2]);
}

static PyObject *match_end(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False;
    if (!PyArg_ParseTuple(args, "|O:end", &index_))
    {
		return NULL;
	}

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) 
	{
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    return Py_BuildValue("i", self->mark[index*2+1]);
}

LOCAL(PyObject*) _pair(int i1, int i2)
{
    PyObject* pair;
    PyObject* item;

    pair = PyTuple_New(2);
    if (!pair)
    {
		return NULL;
	}

    item = PyInt_FromLong(i1);
    if (!item)
    {
		goto error;
    }
	PyTuple_SET_ITEM(pair, 0, item);

    item = PyInt_FromLong(i2);
    if (!item)
    {
		goto error;
    }
	PyTuple_SET_ITEM(pair, 1, item);

    return pair;

error:
    Py_DECREF(pair);
    return NULL;
}

static PyObject *match_span(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False;
    if (!PyArg_ParseTuple(args, "|O:span", &index_))
    {
		return NULL;
	}

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) 
	{
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    return _pair(self->mark[index*2], self->mark[index*2+1]);
}

static PyObject *match_regs(MatchObject* self)
{
    PyObject* regs;
    PyObject* item;
    int index;

    regs = PyTuple_New(self->groups);
    if (!regs)
	{
        return NULL;
	}

    for (index = 0; index < self->groups; index++) 
	{
        item = _pair(self->mark[index*2], self->mark[index*2+1]);
        if (!item) 
		{
            Py_DECREF(regs);
            return NULL;
        }
        PyTuple_SET_ITEM(regs, index, item);
    }

    Py_INCREF(regs);
    self->regs = regs;

    return regs;
}

static PyObject *match_copy(MatchObject* self, PyObject* args)
{
    PyErr_SetString(PyExc_TypeError, "cannot copy this match object");
    return NULL;
}

static PyObject *match_deepcopy(MatchObject* self, PyObject* args)
{
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this match object");
    return NULL;
}

static PyMethodDef match_methods[] = {
    {"group", (PyCFunction) match_group, METH_VARARGS},
    {"start", (PyCFunction) match_start, METH_VARARGS},
    {"end", (PyCFunction) match_end, METH_VARARGS},
    {"span", (PyCFunction) match_span, METH_VARARGS},
    {"groups", (PyCFunction) match_groups, METH_VARARGS|METH_KEYWORDS},
    {"groupdict", (PyCFunction) match_groupdict, METH_VARARGS|METH_KEYWORDS},
    {"expand", (PyCFunction) match_expand, METH_VARARGS},
    {"__copy__", (PyCFunction) match_copy, METH_VARARGS},
    {"__deepcopy__", (PyCFunction) match_deepcopy, METH_VARARGS},
    {NULL, NULL}
};

static PyObject *match_getattr(MatchObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(match_methods, (PyObject*) self, name);
    if (res)
    {
		return res;
	}

    PyErr_Clear();

    if (!strcmp(name, "lastindex")) 
	{
        if (self->lastindex >= 0)
        {
			return Py_BuildValue("i", self->lastindex);
        }
		Py_INCREF(Py_None);
        return Py_None;
    }

    if (!strcmp(name, "lastgroup")) 
	{
        if (self->pattern->indexgroup && self->lastindex >= 0) 
		{
            PyObject* result = PySequence_GetItem(
                self->pattern->indexgroup, self->lastindex
                );
            if (result)
            {
				return result;
            }
			PyErr_Clear();
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!strcmp(name, "string")) 
	{
        if (self->string) 
		{
            Py_INCREF(self->string);
            return self->string;
        } 
		else 
		{
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    if (!strcmp(name, "regs")) 
	{
        if (self->regs) 
		{
            Py_INCREF(self->regs);
            return self->regs;
        } 
		else
		{
			return match_regs(self);
		}
    }

    if (!strcmp(name, "re")) 
	{
        Py_INCREF(self->pattern);
        return (PyObject*) self->pattern;
    }

    if (!strcmp(name, "pos"))
	{
        return Py_BuildValue("i", self->pos);
	}

    if (!strcmp(name, "endpos"))
    {
		return Py_BuildValue("i", self->endpos);
	}

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Match_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "_" SRE_MODULE ".SRE_Match",
    sizeof(MatchObject), sizeof(int),
    (destructor)match_dealloc, 
    0,
    (getattrfunc)match_getattr
};

static void scanner_dealloc(ScannerObject* self)
{
    state_fini(&self->state);
    Py_DECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject *scanner_match(ScannerObject* self, PyObject* args)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    int status;

    state_reset(state);

    state->ptr = state->start;

    if (state->charsize == 1) 
	{
        status = sre_match(state, PatternObject_GetCode(self->pattern), 1);
    } 
	else 
	{
        status = sre_umatch(state, PatternObject_GetCode(self->pattern), 1);
    }

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
	{
        state->start = (void*) ((char*) state->ptr + state->charsize);
    }
	else
    {
		state->start = state->ptr;
	}

    return match;
}


static PyObject *scanner_search(ScannerObject* self, PyObject* args)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    int status;

    state_reset(state);

    state->ptr = state->start;

    if (state->charsize == 1) 
	{
        status = sre_search(state, PatternObject_GetCode(self->pattern));
    } 
	else 
	{
        status = sre_usearch(state, PatternObject_GetCode(self->pattern));
    }

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
	{
        state->start = (void*) ((char*) state->ptr + state->charsize);
    }
	else
    {
		state->start = state->ptr;
	}

    return match;
}

static PyMethodDef scanner_methods[] = {
    {"match", (PyCFunction) scanner_match, 0},
    {"search", (PyCFunction) scanner_search, 0},
    {NULL, NULL}
};

static PyObject *scanner_getattr(ScannerObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(scanner_methods, (PyObject*) self, name);
    if (res)
    {
		return res;
	}

    PyErr_Clear();

    if (!strcmp(name, "pattern")) 
	{
        Py_INCREF(self->pattern);
        return self->pattern;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Scanner_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "_" SRE_MODULE ".SRE_Scanner",
    sizeof(ScannerObject), 0,
    (destructor)scanner_dealloc, 
    0,
    (getattrfunc)scanner_getattr,
};

static PyMethodDef _functions[] = {
    {"compile", _compile, 1},
    {"getcodesize", sre_codesize, 1},
    {"getlower", sre_getlower, 1},
    {NULL, NULL}
};

void init_sre()
{
    PyObject* m;
    PyObject* d;
    PyObject* x;

    Pattern_Type.ob_type = Match_Type.ob_type =
        Scanner_Type.ob_type = &PyType_Type;

    m = Py_InitModule("_" SRE_MODULE, _functions);
    d = PyModule_GetDict(m);

    x = PyInt_FromLong(SRE_MAGIC);
    if (x) 
	{
        PyDict_SetItemString(d, "MAGIC", x);
        Py_DECREF(x);
    }

    x = PyString_FromString(copyright);
    if (x) 
	{
        PyDict_SetItemString(d, "copyright", x);
        Py_DECREF(x);
    }
}
