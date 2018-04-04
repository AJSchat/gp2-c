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
// test.c - Routines used to test the generic parser (either the C or C++ implementation of it).


#ifndef __cplusplus
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "qcommon/q_platform.h"
#include "gp2-c/genericparser2.h"

// C, new API.
#define Test_GP_Parse(a,b,c)                GP_Parse(a)
#define Test_GPG_GetName(a,b,c)             GPG_GetName(a,b,c)
#define Test_GPG_FindPairValue(a,b,c,d,e)   GPG_FindPairValue(a,b,c,d,e)
#define Test_GPV_GetName(a,b,c)             GPV_GetName(a,b,c)
#define Test_GPV_GetTopValue(a,b,c)         GPV_GetTopValue(a,b,c)
#else
extern "C" {
    #include "qcommon/q_shared.h"
    #include "qcommon/qcommon.h"
    #include "qcommon/q_platform.h"
}
#include "gp2-cpp/genericparser2.h"

// C++, original API.
#define Test_GP_Parse(a,b,c)                GP_Parse(a,b,c)
#define Test_GPG_GetName(a,b,c)             GPG_GetName(a,b)
#define Test_GPG_FindPairValue(a,b,c,d,e)   GPG_FindPairValue(a,b,c,d)
#define Test_GPV_GetName(a,b,c)             GPV_GetName(a,b)
#define Test_GPV_GetTopValue(a,b,c)         GPV_GetTopValue(a,b)

/*
==================
GP_ParseFile

Fully parse the specified GP2 file.
==================
*/

static TGenericParser2 GP_ParseFile(char *fileName)
{
    TGenericParser2 GP2;
    char            *dataPtr;
    union {
        char    *c;
        void    *v;
    } buf;

    // Read the specified GP2 file.
    FS_ReadFile(fileName, &buf.v);
    if(!buf.c){
        return NULL;
    }

    // Parse the GP2 file.
    dataPtr = buf.c;
    GP2 = GP_Parse(&dataPtr, qtrue, qfalse);

    // Clean up and return.
    FS_FreeFile(buf.v);
    return GP2;
}
#endif // not __cplusplus

static char *readFile(const char *fileName)
{
    FILE        *f;
    long        fsize;
    static char *fileBuf = NULL;

    // Read input file.
    f = fopen(fileName, "rb");
    if(f == NULL){
        Com_Printf("Couldn't read file %s.\n", fileName);
        return NULL;
    }

    // Determine file size.
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate the required memory and read the file.
    fileBuf = (char *)malloc(fsize + 1);
    fread(fileBuf, fsize, 1, f);
    fclose(f);
    fileBuf[fsize] = '\0';

    return fileBuf;
}

long FS_ReadFile(const char *qpath, void **buffer)
{
    *buffer = readFile(qpath);

    return 0;
}

void FS_FreeFile(void *buffer)
{
    free(buffer);
}

//=============================================

void BG_ParseItemFile()
{
    TGPGroup        baseGroup, subGroup;
    TGPValue        pairs;
    char            temp[1024];
    TGenericParser2 ItemFile;

    // Create the generic parser so the item file can be parsed.
    ItemFile = GP_ParseFile("./testfiles/SOF2.item");
    if(!ItemFile){
        return;
    }

    Com_Printf("\n--> Item file <--\n");

    baseGroup = GP_GetBaseParseGroup(ItemFile);
    subGroup = GPG_GetSubGroups (baseGroup);

    while(subGroup){
        Test_GPG_GetName(subGroup, temp, sizeof(temp));

        if(Q_stricmp(temp, "item") == 0){
            // Is this item used for deathmatch?
            Test_GPG_FindPairValue(subGroup, "Deathmatch", "yes", temp, sizeof(temp));
            if(Q_stricmp(temp, "no") == 0){
                subGroup = GPG_GetNext(subGroup);
                continue;
            }

            Com_Printf("= Item =\n");
            // Name of the item.
            Test_GPG_FindPairValue(subGroup, "Name", "", temp, sizeof(temp));
            Com_Printf("Name: %s\n", temp);

            // Model for the item.
            Test_GPG_FindPairValue(subGroup, "Model", "", temp, sizeof(temp));
            Com_Printf("Model: %s\n", temp[0] ? temp : "-");

            pairs = GPG_GetPairs(subGroup);
            while(pairs){
                Com_Printf("== Pair ==\n");
                Test_GPV_GetName(pairs, temp, sizeof(temp));
                Com_Printf("Name: %s\n", temp);

                // Surface off?
                if(Q_stricmpn(temp, "offsurf", 7) == 0){
                    // Name of the surface to turn off.
                    Test_GPV_GetTopValue(pairs, temp, sizeof(temp));
                    Com_Printf("Surface to turn off: %s\n", temp);
                }
                // Surface on?
                else if(Q_stricmpn(temp, "onsurf", 6) == 0){
                    Test_GPV_GetTopValue(pairs, temp, sizeof(temp));
                    Com_Printf("Surface to turn on: %s\n", temp);
                }

                // Next pairs
                pairs = GPV_GetNext(pairs);
            }
        }

        // Next group
        subGroup = GPG_GetNext ( subGroup );
    }

    GP_Delete(&ItemFile);
}

void BG_ParseGametypeInfo()
{
    TGenericParser2     GP2;
    TGPGroup            topGroup;
    TGPGroup            gtGroup;
    char                temp[1024];

    // Create the generic parser so the item file can be parsed.
    GP2 = GP_ParseFile("./testfiles/ctf.gametype");
    if(!GP2){
        return;
    }

    Com_Printf("\n--> Gametype file <--\n");

    // Top group should only contain the "gametype" sub group.
    topGroup = GP_GetBaseParseGroup(GP2);
    if(!topGroup){
        Com_Printf("No gametype sub group (1)!\n");
        GP_Delete(&GP2);
        return;
    }

    // Grab the gametype sub group.
    gtGroup = GPG_FindSubGroup(topGroup, "gametype");
    if(!gtGroup){
        Com_Printf("No gametype sub group (2)!");
        GP_Delete(&GP2);
        return;
    }

    // Parse out the name of the gametype.
    Test_GPG_FindPairValue(gtGroup, "displayname", "", temp, sizeof(temp));
    if(!temp[0]){
        Com_Printf("No display name!\n");
        GP_Delete(&GP2);
        return;
    }
    Com_Printf("Display name: %s\n", temp);

    // Gametype description.
    Test_GPG_FindPairValue(gtGroup, "description", "", temp, sizeof(temp));
    Com_Printf("Description: %s\n", temp[0] ? temp : "-");

    // Are pickups enabled?
    Test_GPG_FindPairValue(gtGroup, "pickups", "yes", temp, sizeof(temp));
    Com_Printf("Pickups enabled: %s\n",!Q_stricmp(temp, "no") ? "no" : "yes");

    // Are teams enabled?
    Test_GPG_FindPairValue(gtGroup, "teams", "yes", temp, sizeof(temp));
    Com_Printf("Teams enabled: %s\n",!Q_stricmp(temp, "yes") ? "yes" : "no");

    // Display kills.
    Test_GPG_FindPairValue(gtGroup, "showkills", "no", temp, sizeof(temp));
    Com_Printf("Show kills: %s\n",!Q_stricmp(temp, "yes") ? "yes" : "no");

    // Look for the respawn type.
    Test_GPG_FindPairValue(gtGroup, "respawn", "normal", temp, sizeof(temp));
    Com_Printf("Respawn type: %s\n", Q_stricmp(temp, "none") && Q_stricmp(temp, "interval") ? "normal" : temp);

    // A gametype can be based off another gametype which means it uses all the gametypes entities.
    Test_GPG_FindPairValue(gtGroup, "basegametype", "", temp, sizeof(temp));
    Com_Printf("Base gametype: %s\n", temp[0] ? temp : "-");

    // What percentage does the backpack replenish?
    Test_GPG_FindPairValue(gtGroup, "backpack", "0", temp, sizeof(temp));
    Com_Printf("Backpack replenish: %d\n", atoi(temp));

    // Cleanup the generic parser.
    GP_Delete(&GP2);
}

void BG_ParseNPCFile()
{
    TGenericParser2     NPCFile;
    TGPGroup            baseGroup, subGroup, soundsGroup, pairs;
    TGPValue            list;
    char                temp[1024];

    // Create the generic parser so the item file can be parsed.
    NPCFile = GP_ParseFile("./testfiles/Base.npc");
    if(!NPCFile){
        return;
    }

    Com_Printf("\n--> NPC file <--\n");

    baseGroup = GP_GetBaseParseGroup(NPCFile);
    subGroup = GPG_GetSubGroups(baseGroup);

    while(subGroup){
        Test_GPG_GetName(subGroup, temp, sizeof(temp));

        // A new character template.
        if(Q_stricmp(temp, "CharacterTemplate") == 0){
            // Parse the sounds for this character template.
            soundsGroup = GPG_FindSubGroup(subGroup, "MPSounds");
            if(!soundsGroup){
                subGroup = GPG_GetNext(subGroup);
                continue;
            }

            // Now parse all the skin groups.
            pairs = GPG_GetPairs(soundsGroup);
            while(pairs){
                Com_Printf("= Sound =\n");

                // Grab the sounds name.
                Test_GPV_GetName(pairs, temp, sizeof(temp));
                if(temp[0]){
                    Com_Printf("Name: %s\n", temp);
                }

                // Should be a list.
                if(GPV_IsList(pairs)){
                    Com_Printf("== List ==\n");

                    // Run through the list.
                    list = GPV_GetList(pairs);
                    while(list){
                        Test_GPV_GetName(list, temp, sizeof(temp));
                        if(temp[0]){
                            Com_Printf("Sound in list: %s\n", temp);
                        }

                        list = GPV_GetNext(list);
                    }
                }

                // Move to the next sound set in the parsers list.
                pairs = GPV_GetNext(pairs);
            }
        }

        // Move to the next group.
        subGroup = GPG_GetNext(subGroup);
    }

    // Cleanup the generic parser.
    GP_Delete(&NPCFile);
}
