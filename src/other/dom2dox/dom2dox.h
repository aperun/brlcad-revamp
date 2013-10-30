#ifndef ON2JAVA_H
#define ON2JAVA_H

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include "bu.h"

typedef struct {
    struct bu_vls value;
} token_t;

typedef struct {
    token_t *tokenData;
    FILE *outfile;
    int comment_type;
    int doc_comment;
    int tag_text;
    int example_text;
    int return_text;
    struct bu_vls description;
    struct bu_vls tags;
} app_data_t;

/* lemon prototypes */
void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *parser, void (*freeProc)(void *));
void Parse(void *yyp, int yymajor, token_t *tokenData, app_data_t *appData);
void ParseTrace(FILE *fp, char *s);

/* definitions generated by lemon */
#include "dom2dox_parser.h"

#define PERPLEX_ON_ENTER \
    app_data_t *appData = (app_data_t *)yyextra; \
    token_t *tokenData = appData->tokenData;

/* definitions generated by perplex */
#include "dom2dox_scanner.h"

/* utils */
#define END_EXAMPLE \
if (appData->example_text) { \
    bu_vls_strcat(&appData->description, "\n\\endcode\n\n"); \
    appData->example_text = 0; \
}

#endif
