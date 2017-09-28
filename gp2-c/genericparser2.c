/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors
Copyright (C) 2017, Ane-Jouke Schat

This file is part of the gp2-c source code.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/
// genericparser2.c - Main Generic Parser 2 file.

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "genericparser2.h"

// Local function definitions.
static qboolean     GPG_Parse               ( CGPGroup *group, char **dataPtr, CTextPool **textPool );
static CGPGroup     *GPG_AddGroup           ( CGPGroup *parent, char *name, CTextPool **textPool );
static CGPValue     *GPG_AddPair            ( CGPGroup *parent, char *name, char *value, CTextPool **textPool );

static qboolean     GPV_Parse               ( CGPValue *gpv, char **dataPtr, CTextPool **textPool );
static void         GPV_AddValue            ( CGPValue *gpv, char *newValue, CTextPool **textPool );

static char         *AllocText              ( CTextPool *textPool, char *text, qboolean addNULL, CTextPool **poolPtr );
static char         *GetToken               ( char **text, qboolean allowLineBreaks, qboolean readUntilEOL );
static void         SortObject              ( void *object, void **unsortedList, void **sortedList, void **lastObject );

// Local variable definitions.
static char         token[MAX_TOKEN_SIZE];

/*
==================
GP_Parse

Fully parse a GP2 data buffer.
The cleanFirst and writeable variables are deprecated and not used anywhere,
but are kept for the sake of compatibility.
==================
*/

TGenericParser2 GP_Parse(char **dataPtr, qboolean cleanFirst, qboolean writeable)
{
    CGenericParser2     *topLevel;
    CTextPool           *topPool;

    // Allocate and zero initialize the main parser structure.
    topLevel = calloc(1, sizeof(CGenericParser2));

    // Allocate and zero initialize the text pool.
    topLevel->mTextPool = calloc(1, sizeof(CTextPool));
    topPool = topLevel->mTextPool;

    // Allocate memory for the actual pool.
    topPool->mSize = TOPPOOL_SIZE;
    topPool->mPool = Z_TagMalloc(topPool->mSize, TAG_TEXTPOOL);
    Com_Memset(topPool->mPool, 0, topPool->mSize);

    // Start parsing groups.
    topLevel->mTopLevel.mBase.mName = "Top Level";
    if(GPG_Parse(&topLevel->mTopLevel, dataPtr, &topPool)){
         // Successful, return the end result.
        return topLevel;
    }

    // Unsuccessful, clean up and return.
    GP_Clean(topLevel);
    return 0;
}

/*
==================
GP_Clean

Cleans GP2 instance.
==================
*/

void GP_Clean(TGenericParser2 GP2)
{
    CGenericParser2 *CGP2;
    CGPGroup *topLevel;

    if(!GP2){
        return;
    }

    CGP2 = GP2;
    topLevel = &CGP2->mTopLevel;

    // Free all allocated pairs.
    while(topLevel->mPairs){
        topLevel->mCurrentPair = topLevel->mPairs->mBase.mNext;
        free(topLevel->mPairs);
        topLevel->mPairs = topLevel->mCurrentPair;
    }

    // Free all allocated subgroups.
    while(topLevel->mSubGroups)
    {
        topLevel->mCurrentSubGroup = topLevel->mSubGroups->mBase.mNext;
        free(topLevel->mSubGroups);
        topLevel->mSubGroups = topLevel->mCurrentSubGroup;
    }

    topLevel->mPairs = topLevel->mInOrderPairs = topLevel->mCurrentPair = NULL;
    topLevel->mSubGroups = topLevel->mInOrderSubGroups = topLevel->mCurrentSubGroup = NULL;
    topLevel->mParent = NULL;
    topLevel->mWriteable = qfalse;
}

/*
==================
GPG_Parse

Parse a GP2 group.
==================
*/

static qboolean GPG_Parse(CGPGroup *group, char **dataPtr, CTextPool **textPool)
{
    char        *token;
    char        lastToken[MAX_TOKEN_SIZE];
    CGPGroup    *newSubGroup;
    CGPValue    *newPair;

    while(1){
        token = GetToken(dataPtr, qtrue, qfalse);

        if(!token[0]){
            // End of data - error!
            if(group->mParent){
                return qfalse;
            }

            break;
        }else if(Q_stricmp(token, "}") == 0){
            // Ending brace for this group.
            break;
        }

        strncpy(lastToken, token, sizeof(lastToken));

        // Read ahead to see what we are doing.
        token = GetToken(dataPtr, qtrue, qtrue);
        if(Q_stricmp(token, "{") == 0){
            // New sub group - add it.
            newSubGroup = GPG_AddGroup(group, lastToken, textPool);

            // Parse data.
            if(!GPG_Parse(newSubGroup, dataPtr, textPool)){
                return qfalse;
            }
        }else if(Q_stricmp(token, "[") == 0){
            // New pair list.
            newPair = GPG_AddPair(group, lastToken, 0, textPool);
            if(!GPV_Parse(newPair, dataPtr, textPool)){
                return qfalse;
            }
        }else{
            // New pair.
            GPG_AddPair(group, lastToken, token, textPool);
        }
    }

    return qtrue;
}

/*
==================
GPG_AddGroup

Adds a GP2 group.
==================
*/

static CGPGroup *GPG_AddGroup(CGPGroup *parent, char *name, CTextPool **textPool)
{
    CGPGroup    *newGroup;

    if(textPool){
        name = AllocText((*textPool), name, qtrue, textPool);
    }

    // Allocate memory for new group.
    newGroup = calloc(1, sizeof(CGPGroup));
    newGroup->mParent = parent;

    // Set proper name.
    newGroup->mBase.mName = name;

    // Update sorting.
    SortObject(newGroup, (void **)&parent->mSubGroups,
        (void **)&parent->mInOrderSubGroups,
        (void **)&parent->mCurrentSubGroup
    );

    return newGroup;
}

/*
==================
GPG_AddPair

Adds a GP2 value based on a pair (name/value).
==================
*/

static CGPValue *GPG_AddPair(CGPGroup *parent, char *name, char *value, CTextPool **textPool)
{
    CGPValue *newPair;

    if(textPool){
        name = AllocText((*textPool), name, qtrue, textPool);
        if(value){
            value = AllocText((*textPool), value, qtrue, textPool);
        }
    }

    newPair = calloc(1, sizeof(CGPValue));

    SortObject(newPair, (void **)&parent->mPairs,
        (void **)&parent->mInOrderPairs,
        (void **)&parent->mCurrentPair);

    return newPair;
}

/*
==================
GPV_Parse

Parse a GP2 value.
==================
*/

static qboolean GPV_Parse(CGPValue *gpv, char **dataPtr, CTextPool **textPool)
{
    char        *token;
    char        *val;

    while(1){
        token = GetToken(dataPtr, qtrue, qtrue);

        if(!token[0]){
            // End of data - error!
            return qfalse;
        }else if(Q_stricmp(token, "]") == 0){
            // Ending brace for this list.
            break;
        }

        val = AllocText((*textPool), token, qtrue, textPool);
        GPV_AddValue(gpv, val, NULL);
    }

    return qtrue;
}

/*
==================
GPV_AddValue

Adds the new value to the list.
==================
*/

static void GPV_AddValue(CGPValue *gpv, char *newValue, CTextPool **textPool)
{
    if(textPool){
        newValue = AllocText((*textPool), newValue, qtrue, textPool);
    }

    if(gpv->mList == NULL){
        gpv->mList = calloc(1, sizeof(CGPValue));
        gpv->mList->mBase.mName == newValue;
        gpv->mList->mBase.mInOrderNext = gpv->mList;
    }else{
        ((CGPValue *)gpv->mBase.mInOrderNext)->mBase.mNext = calloc(1, sizeof(CGPValue));
        gpv->mList->mBase.mInOrderNext = ((CGPValue *)gpv->mList->mBase.mInOrderNext)->mBase.mNext;
    }
}

/*
==================
AllocText

Allocates text and returns a char pointer.
==================
*/

static char *AllocText(CTextPool *textPool, char *text, qboolean addNULL, CTextPool **poolPtr)
{
    int length = strlen(text) + (addNULL ? 1 : 0);

    if(textPool->mUsed + length + 1 > textPool->mSize)
    {
        // Extra 1 to put a null on the end.
        if(poolPtr){
            (*poolPtr)->mNext = calloc(textPool->mSize, sizeof(CTextPool));
            *poolPtr = (*poolPtr)->mNext;

            return AllocText((*poolPtr), text, addNULL, NULL);
        }

        return 0;
    }

    strncpy(textPool->mPool + textPool->mUsed, text, textPool->mSize - textPool->mUsed); // FIXME BOE review for truncation.
    textPool->mUsed += length;
    textPool->mPool[textPool->mUsed] = 0;

    return textPool->mPool + textPool->mUsed - length;
}

/*
==================
GetToken

Parses token from the text buffer. The result is stored in the local
"token" char buffer. A pointer pointing to this buffer is returned.
==================
*/

static char *GetToken(char **text, qboolean allowLineBreaks, qboolean readUntilEOL)
{
    char        *pointer = *text;
    int         length = 0;
    int         c = 0;
    qboolean    foundLineBreak;

    token[0] = 0;
    if (!pointer){
        return token;
    }

    while(1){
        foundLineBreak = qfalse;
        while(1){
            c = *pointer;
            if(c > ' '){
                break;
            }

            if(!c){
                *text = 0;
                return token;
            }

            if(c == '\n'){
                foundLineBreak = qtrue;
            }

            pointer++;
        }

        if(foundLineBreak && !allowLineBreaks){
            *text = pointer;
            return token;
        }

        c = *pointer;

        if(c == '/' && pointer[1] == '/'){
            // Skip single line comment.
            pointer += 2;
            while(*pointer && *pointer != '\n'){
                pointer++;
            }
        }else if(c == '/' && pointer[1] == '*'){
            // Skip multi line comments.
            pointer += 2;
            while(*pointer && (*pointer != '*' || pointer[1] != '/')){
                pointer++;
            }
            if(*pointer){
                pointer += 2;
            }
        }else{
            // Found the start of a token.
            break;
        }
    }

    if(c == '\"'){
        // Handle a string.
        pointer++;
        while(1){
            c = *pointer++;
            if(c == '\"'){
                break;
            }else if(!c){
                break;
            }else if(length < MAX_TOKEN_SIZE){
                token[length++] = c;
            }
        }
    }else if(readUntilEOL){
        // Absorb all characters until EOL.
        while(c != '\n' && c != '\r'){
            if(c == '/' && ((*(pointer+1)) == '/' || (*(pointer+1)) == '*')){
                break;
            }

            if(length < MAX_TOKEN_SIZE){
                token[length++] = c;
            }
            pointer++;
            c = *pointer;
        }
        // Remove trailing white space.
        while(length && token[length-1] < ' '){
            length--;
        }
    }else{
        while(c > ' '){
            if(length < MAX_TOKEN_SIZE){
                token[length++] = c;
            }
            pointer++;
            c = *pointer;
        }
    }

    if(token[0] == '\"'){
        // Remove start quote.
        length--;
        memmove(token, token+1, length);

        if(length && token[length-1] == '\"'){
            // Remove end quote.
            length--;
        }
    }

    if(length >= MAX_TOKEN_SIZE){
        length = 0;
    }
    token[length] = 0;
    *text = (char *)pointer;

    return token;
}

/*
==================
SortObject

Sorts all objects in the given list.
==================
*/

static void SortObject(
    void *object,
    void **unsortedList,
    void **sortedList,
    void **lastObject)
{
    CGPObject   *test, *last;

    if(!*unsortedList){
        *unsortedList = *sortedList = object;
    }else{
        ((CGPObject *)*lastObject)->mNext = object;

        test = *sortedList;
        last = 0;
        while(test){
            if(Q_stricmp(((CGPObject *)object)->mName, test->mName) < 0){
                break;
            }

            last = test;
            test = test->mInOrderNext;
        }

        if(test){
            test->mInOrderPrevious = object;
            ((CGPObject *)object)->mInOrderNext = test;
        }
        if(last){
            last->mInOrderNext = object;
            ((CGPObject *)object)->mInOrderPrevious = last;
        }else{
            *sortedList = object;
        }
    }

    *lastObject = object;
}