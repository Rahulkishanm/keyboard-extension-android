// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*
*   Copyright (C) 2005-2012, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  writesrc.c
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005apr23
*   created by: Markus W. Scherer
*
*   Helper functions for writing source code for data.
*/

#include <stdio.h>
#include <time.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ucptrie.h"
#include "utrie2.h"
#include "cstring.h"
#include "writesrc.h"

static FILE *
usrc_createWithHeader(const char *path, const char *filename,
                      const char *header, const char *generator) {
    char buffer[1024];
    const char *p;
    char *q;
    FILE *f;
    char c;

    if(path==NULL) {
        p=filename;
    } else {
        /* concatenate path and filename, with U_FILE_SEP_CHAR in between if necessary */
        uprv_strcpy(buffer, path);
        q=buffer+uprv_strlen(buffer);
        if(q>buffer && (c=*(q-1))!=U_FILE_SEP_CHAR && c!=U_FILE_ALT_SEP_CHAR) {
            *q++=U_FILE_SEP_CHAR;
        }
        uprv_strcpy(q, filename);
        p=buffer;
    }

    f=fopen(p, "w");
    if(f!=NULL) {
        const struct tm *lt;
        time_t t;

        time(&t);
        lt=localtime(&t);
        if(generator==NULL) {
            strftime(buffer, sizeof(buffer), "%Y-%m-%d", lt);
            fprintf(f, header, filename, buffer);
        } else {
            fprintf(f, header, filename, generator);
        }
    } else {
        fprintf(
            stderr,
            "usrc_create(%s, %s): unable to create file\n",
            path!=NULL ? path : "", filename);
    }
    return f;
}

U_CAPI FILE * U_EXPORT2
usrc_create(const char *path, const char *filename, int32_t copyrightYear, const char *generator) {
    const char *header;
    char buffer[200];
    if(copyrightYear<=2016) {
        header=
            "// © 2016 and later: Unicode, Inc. and others.\n"
            "// License & terms of use: http://www.unicode.org/copyright.html\n"
            "//\n"
            "// Copyright (C) 1999-2016, International Business Machines\n"
            "// Corporation and others.  All Rights Reserved.\n"
            "//\n"
            "// file name: %s\n"
            "//\n"
            "// machine-generated by: %s\n"
            "\n\n";
    } else {
        sprintf(buffer,
                "// © %d and later: Unicode, Inc. and others.\n"
                "// License & terms of use: http://www.unicode.org/copyright.html\n"
                "//\n"
                "// file name: %%s\n"
                "//\n"
                "// machine-generated by: %%s\n"
                "\n\n",
                (int)copyrightYear);
        header=buffer;
    }
    return usrc_createWithHeader(path, filename, header, generator);
}

U_CAPI FILE * U_EXPORT2
usrc_createTextData(const char *path, const char *filename, const char *generator) {
    // TODO: Add parameter for the first year this file was generated, not before 2016.
    static const char *header=
        "# Copyright (C) 2016 and later: Unicode, Inc. and others.\n"
        "# License & terms of use: http://www.unicode.org/copyright.html\n"
        "# Copyright (C) 1999-2016, International Business Machines\n"
        "# Corporation and others.  All Rights Reserved.\n"
        "#\n"
        "# file name: %s\n"
        "#\n"
        "# machine-generated by: %s\n"
        "\n\n";
    return usrc_createWithHeader(path, filename, header, generator);
}

U_CAPI void U_EXPORT2
usrc_writeArray(FILE *f,
                const char *prefix,
                const void *p, int32_t width, int32_t length,
                const char *postfix) {
    const uint8_t *p8;
    const uint16_t *p16;
    const uint32_t *p32;
    uint32_t value;
    int32_t i, col;

    p8=NULL;
    p16=NULL;
    p32=NULL;
    switch(width) {
    case 8:
        p8=(const uint8_t *)p;
        break;
    case 16:
        p16=(const uint16_t *)p;
        break;
    case 32:
        p32=(const uint32_t *)p;
        break;
    default:
        fprintf(stderr, "usrc_writeArray(width=%ld) unrecognized width\n", (long)width);
        return;
    }
    if(prefix!=NULL) {
        fprintf(f, prefix, (long)length);
    }
    for(i=col=0; i<length; ++i, ++col) {
        if(i>0) {
            if(col<16) {
                fputc(',', f);
            } else {
                fputs(",\n", f);
                col=0;
            }
        }
        switch(width) {
        case 8:
            value=p8[i];
            break;
        case 16:
            value=p16[i];
            break;
        case 32:
            value=p32[i];
            break;
        default:
            value=0; /* unreachable */
            break;
        }
        fprintf(f, value<=9 ? "%lu" : "0x%lx", (unsigned long)value);
    }
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUTrie2Arrays(FILE *f,
                       const char *indexPrefix, const char *data32Prefix,
                       const UTrie2 *pTrie,
                       const char *postfix) {
    if(pTrie->data32==NULL) {
        /* 16-bit trie */
        usrc_writeArray(f, indexPrefix, pTrie->index, 16, pTrie->indexLength+pTrie->dataLength, postfix);
    } else {
        /* 32-bit trie */
        usrc_writeArray(f, indexPrefix, pTrie->index, 16, pTrie->indexLength, postfix);
        usrc_writeArray(f, data32Prefix, pTrie->data32, 32, pTrie->dataLength, postfix);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUTrie2Struct(FILE *f,
                       const char *prefix,
                       const UTrie2 *pTrie,
                       const char *indexName, const char *data32Name,
                       const char *postfix) {
    if(prefix!=NULL) {
        fputs(prefix, f);
    }
    if(pTrie->data32==NULL) {
        /* 16-bit trie */
        fprintf(
            f,
            "    %s,\n"         /* index */
            "    %s+%ld,\n"     /* data16 */
            "    NULL,\n",      /* data32 */
            indexName,
            indexName, 
            (long)pTrie->indexLength);
    } else {
        /* 32-bit trie */
        fprintf(
            f,
            "    %s,\n"         /* index */
            "    NULL,\n"       /* data16 */
            "    %s,\n",        /* data32 */
            indexName,
            data32Name);
    }
    fprintf(
        f,
        "    %ld,\n"            /* indexLength */
        "    %ld,\n"            /* dataLength */
        "    0x%hx,\n"          /* index2NullOffset */
        "    0x%hx,\n"          /* dataNullOffset */
        "    0x%lx,\n"          /* initialValue */
        "    0x%lx,\n"          /* errorValue */
        "    0x%lx,\n"          /* highStart */
        "    0x%lx,\n"          /* highValueIndex */
        "    NULL, 0, FALSE, FALSE, 0, NULL\n",
        (long)pTrie->indexLength, (long)pTrie->dataLength,
        (short)pTrie->index2NullOffset, (short)pTrie->dataNullOffset,
        (long)pTrie->initialValue, (long)pTrie->errorValue,
        (long)pTrie->highStart, (long)pTrie->highValueIndex);
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUCPTrieArrays(FILE *f,
                        const char *indexPrefix, const char *dataPrefix,
                        const UCPTrie *pTrie,
                        const char *postfix) {
    usrc_writeArray(f, indexPrefix, pTrie->index, 16, pTrie->indexLength, postfix);
    int32_t width=
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_16 ? 16 :
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_32 ? 32 :
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_8 ? 8 : 0;
    usrc_writeArray(f, dataPrefix, pTrie->data.ptr0, width, pTrie->dataLength, postfix);
}

U_CAPI void U_EXPORT2
usrc_writeUCPTrieStruct(FILE *f,
                        const char *prefix,
                        const UCPTrie *pTrie,
                        const char *indexName, const char *dataName,
                        const char *postfix) {
    if(prefix!=NULL) {
        fputs(prefix, f);
    }
    fprintf(
        f,
        "    %s,\n"             // index
        "    { %s },\n",        // data (union)
        indexName,
        dataName);
    fprintf(
        f,
        "    %ld, %ld,\n"       // indexLength, dataLength
        "    0x%lx, 0x%x,\n"    // highStart, shifted12HighStart
        "    %d, %d,\n"         // type, valueWidth
        "    0, 0,\n"           // reserved32, reserved16
        "    0x%x, 0x%lx,\n"    // index3NullOffset, dataNullOffset
        "    0x%lx,\n",         // nullValue
        (long)pTrie->indexLength, (long)pTrie->dataLength,
        (long)pTrie->highStart, pTrie->shifted12HighStart,
        pTrie->type, pTrie->valueWidth,
        pTrie->index3NullOffset, (long)pTrie->dataNullOffset,
        (long)pTrie->nullValue);
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUCPTrie(FILE *f, const char *name, const UCPTrie *pTrie) {
    int32_t width=
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_16 ? 16 :
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_32 ? 32 :
        pTrie->valueWidth==UCPTRIE_VALUE_BITS_8 ? 8 : 0;
    char line[100], line2[100], line3[100];
    sprintf(line, "static const uint16_t %s_trieIndex[%%ld]={\n", name);
    sprintf(line2, "static const uint%d_t %s_trieData[%%ld]={\n", (int)width, name);
    usrc_writeUCPTrieArrays(f, line, line2, pTrie, "\n};\n\n");
    sprintf(line, "static const UCPTrie %s_trie={\n", name);
    sprintf(line2, "%s_trieIndex", name);
    sprintf(line3, "%s_trieData", name);
    usrc_writeUCPTrieStruct(f, line, pTrie, line2, line3, "};\n\n");
}

U_CAPI void U_EXPORT2
usrc_writeArrayOfMostlyInvChars(FILE *f,
                                const char *prefix,
                                const char *p, int32_t length,
                                const char *postfix) {
    int32_t i, col;
    int prev2, prev, c;

    if(prefix!=NULL) {
        fprintf(f, prefix, (long)length);
    }
    prev2=prev=-1;
    for(i=col=0; i<length; ++i, ++col) {
        c=(uint8_t)p[i];
        if(i>0) {
            /* Break long lines. Try to break at interesting places, to minimize revision diffs. */
            if( 
                /* Very long line. */
                col>=32 ||
                /* Long line, break after terminating NUL. */
                (col>=24 && prev2>=0x20 && prev==0) ||
                /* Medium-long line, break before non-NUL, non-character byte. */
                (col>=16 && (prev==0 || prev>=0x20) && 0<c && c<0x20)
            ) {
                fputs(",\n", f);
                col=0;
            } else {
                fputc(',', f);
            }
        }
        fprintf(f, c<0x20 ? "%u" : "'%c'", c);
        prev2=prev;
        prev=c;
    }
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}
