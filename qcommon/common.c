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
// common.c -- misc functions used in client and server

#include "q_shared.h"
#include "qcommon.h"

#define MIN_DEDICATED_COMHUNKMEGS 1
#define MIN_COMHUNKMEGS     56
#define DEF_COMHUNKMEGS     128
#define DEF_COMZONEMEGS     24
#define DEF_COMHUNKMEGS_S   XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S   XSTRING(DEF_COMZONEMEGS)

static  int     s_zoneTotal;
static  int     s_smallZoneTotal;

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
    va_list     argptr;
    char        msg[MAXPRINTMSG];

    va_start (argptr,fmt);
    Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
    va_end (argptr);

    // echo to dedicated console and early console
    printf( "%s", msg );
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf( const char *fmt, ...) {
    va_list     argptr;
    char        msg[MAXPRINTMSG];

    va_start (argptr,fmt);
    Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
    va_end (argptr);

    Com_Printf ("%s", msg);
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate thing.
=============
*/
void QDECL Com_Error( int code, const char *fmt, ... ) {
    va_list     argptr;
    char        msg[MAXPRINTMSG];

    va_start (argptr,fmt);
    Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
    va_end (argptr);

    Com_Printf ("%s", msg);

    exit(1);
}

/*
==============================================================================

                        ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define ZONEID  0x1d4a11
#define MINFRAGMENT 64

typedef struct zonedebug_s {
    char *label;
    char *file;
    int line;
    int allocSize;
} zonedebug_t;

typedef struct memblock_s {
    int     size;           // including the header and possibly tiny fragments
    int     tag;            // a tag of 0 is a free block
    struct memblock_s       *next, *prev;
    int     id;             // should be ZONEID
#ifdef ZONE_DEBUG
    zonedebug_t d;
#endif
} memblock_t;

typedef struct {
    int     size;           // total bytes malloced, including header
    int     used;           // total bytes used
    memblock_t  blocklist;  // start / end cap for linked list
    memblock_t  *rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
memzone_t   *mainzone;
// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
memzone_t   *smallzone;

void Z_CheckHeap( void );

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t *zone, int size ) {
    memblock_t  *block;

    // set the entire zone to one free block

    zone->blocklist.next = zone->blocklist.prev = block =
        (memblock_t *)( (byte *)zone + sizeof(memzone_t) );
    zone->blocklist.tag = 1;    // in use block
    zone->blocklist.id = 0;
    zone->blocklist.size = 0;
    zone->rover = block;
    zone->size = size;
    zone->used = 0;

    block->prev = block->next = &zone->blocklist;
    block->tag = 0;         // free block
    block->id = ZONEID;
    block->size = size - sizeof(memzone_t);
}

/*
========================
Z_AvailableZoneMemory
========================
*/
int Z_AvailableZoneMemory( memzone_t *zone ) {
    return zone->size - zone->used;
}

/*
========================
Z_AvailableMemory
========================
*/
int Z_AvailableMemory( void ) {
    return Z_AvailableZoneMemory( mainzone );
}

/*
========================
Z_Free
========================
*/
void Z_Free( void *ptr ) {
    memblock_t  *block, *other;
    memzone_t *zone;

    if (!ptr) {
        Com_Error( ERR_DROP, "Z_Free: NULL pointer" );
    }

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
    if (block->id != ZONEID) {
        Com_Error( ERR_FATAL, "Z_Free: freed a pointer without ZONEID" );
    }
    if (block->tag == 0) {
        Com_Error( ERR_FATAL, "Z_Free: freed a freed pointer" );
    }
    // if static memory
    if (block->tag == TAG_STATIC) {
        return;
    }

    // check the memory trash tester
    if ( *(int *)((byte *)block + block->size - 4 ) != ZONEID ) {
        Com_Error( ERR_FATAL, "Z_Free: memory block wrote past end" );
    }

    if (block->tag == TAG_SMALL) {
        zone = smallzone;
    }
    else {
        zone = mainzone;
    }

    zone->used -= block->size;
    // set the block to something that should cause problems
    // if it is referenced...
    Com_Memset( ptr, 0xaa, block->size - sizeof( *block ) );

    block->tag = 0;     // mark as free

    other = block->prev;
    if (!other->tag) {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;
        if (block == zone->rover) {
            zone->rover = other;
        }
        block = other;
    }

    zone->rover = block;

    other = block->next;
    if ( !other->tag ) {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;
    }
}


/*
================
Z_FreeTags
================
*/
void Z_FreeTags( int tag ) {
    memzone_t   *zone;

    if ( tag == TAG_SMALL ) {
        zone = smallzone;
    }
    else {
        zone = mainzone;
    }
    // use the rover as our pointer, because
    // Z_Free automatically adjusts it
    zone->rover = zone->blocklist.next;
    do {
        if ( zone->rover->tag == tag ) {
            Z_Free( (void *)(zone->rover + 1) );
            continue;
        }
        zone->rover = zone->rover->next;
    } while ( zone->rover != &zone->blocklist );
}


/*
================
Z_TagMalloc
================
*/
#ifdef ZONE_DEBUG
void *Z_TagMallocDebug( int size, int tag, char *label, char *file, int line ) {
    int     allocSize;
#else
void *Z_TagMalloc( int size, int tag ) {
#endif
    int     extra;
    memblock_t  *start, *rover, *newBlock, *base;
    memzone_t *zone;

    if (!tag) {
        Com_Error( ERR_FATAL, "Z_TagMalloc: tried to use a 0 tag" );
    }

    if ( tag == TAG_SMALL ) {
        zone = smallzone;
    }
    else {
        zone = mainzone;
    }

#ifdef ZONE_DEBUG
    allocSize = size;
#endif
    //
    // scan through the block list looking for the first free block
    // of sufficient size
    //
    size += sizeof(memblock_t); // account for size of block header
    size += 4;                  // space for memory trash tester
    size = PAD(size, sizeof(intptr_t));     // align to 32/64 bit boundary

    base = rover = zone->rover;
    start = base->prev;

    do {
        if (rover == start) {
            // scaned all the way around the list
#ifdef ZONE_DEBUG
            Z_LogHeap();

            Com_Error(ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone: %s, line: %d (%s)",
                                size, zone == smallzone ? "small" : "main", file, line, label);
#else
            Com_Error(ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone",
                                size, zone == smallzone ? "small" : "main");
#endif
            return NULL;
        }
        if (rover->tag) {
            base = rover = rover->next;
        } else {
            rover = rover->next;
        }
    } while (base->tag || base->size < size);

    //
    // found a block big enough
    //
    extra = base->size - size;
    if (extra > MINFRAGMENT) {
        // there will be a free fragment after the allocated block
        newBlock = (memblock_t *) ((byte *)base + size );
        newBlock->size = extra;
        newBlock->tag = 0;           // free block
        newBlock->prev = base;
        newBlock->id = ZONEID;
        newBlock->next = base->next;
        newBlock->next->prev = newBlock;
        base->next = newBlock;
        base->size = size;
    }

    base->tag = tag;            // no longer a free block

    zone->rover = base->next;   // next allocation will start looking here
    zone->used += base->size;   //

    base->id = ZONEID;

#ifdef ZONE_DEBUG
    base->d.label = label;
    base->d.file = file;
    base->d.line = line;
    base->d.allocSize = allocSize;
#endif

    // marker for memory trash testing
    *(int *)((byte *)base + base->size - 4) = ZONEID;

    return (void *) ((byte *)base + sizeof(memblock_t));
}

/*
========================
Z_Malloc
========================
*/
#ifdef ZONE_DEBUG
void *Z_MallocDebug( int size, char *label, char *file, int line ) {
#else
void *Z_Malloc( int size ) {
#endif
    void    *buf;

  //Z_CheckHeap (); // DEBUG

#ifdef ZONE_DEBUG
    buf = Z_TagMallocDebug( size, TAG_GENERAL, label, file, line );
#else
    buf = Z_TagMalloc( size, TAG_GENERAL );
#endif
    Com_Memset( buf, 0, size );

    return buf;
}

#ifdef ZONE_DEBUG
void *S_MallocDebug( int size, char *label, char *file, int line ) {
    return Z_TagMallocDebug( size, TAG_SMALL, label, file, line );
}
#else
void *S_Malloc( int size ) {
    return Z_TagMalloc( size, TAG_SMALL );
}
#endif

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap( void ) {
    memblock_t  *block;

    for (block = mainzone->blocklist.next ; ; block = block->next) {
        if (block->next == &mainzone->blocklist) {
            break;          // all blocks have been hit
        }
        if ( (byte *)block + block->size != (byte *)block->next)
            Com_Error( ERR_FATAL, "Z_CheckHeap: block size does not touch the next block" );
        if ( block->next->prev != block) {
            Com_Error( ERR_FATAL, "Z_CheckHeap: next block doesn't have proper back link" );
        }
        if ( !block->tag && !block->next->tag ) {
            Com_Error( ERR_FATAL, "Z_CheckHeap: two consecutive free blocks" );
        }
    }
}

/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitSmallZoneMemory( void ) {
    s_smallZoneTotal = 512 * 1024;
    smallzone = (memzone_t *)calloc( s_smallZoneTotal, 1 );
    if ( !smallzone ) {
        Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1f megs", (float)s_smallZoneTotal / (1024*1024) );
    }

    Com_Printf("Allocated %1.f megs for small zone data.\n", (float)s_smallZoneTotal / (1024*1024));
    Z_ClearZone( smallzone, s_smallZoneTotal );
}

void Com_InitZoneMemory( void ) {
    s_zoneTotal = 1024 * 1024 * DEF_COMZONEMEGS;

    mainzone = (memzone_t *)calloc( s_zoneTotal, 1 );
    if ( !mainzone ) {
        Com_Error( ERR_FATAL, "Zone data failed to allocate %i megs", s_zoneTotal / (1024*1024) );
    }

    Com_Printf("Allocated %d megs for zone data.\n", s_zoneTotal / (1024 * 1024));
    Z_ClearZone( mainzone, s_zoneTotal );
}

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine ) {
    Com_InitSmallZoneMemory();
    Com_InitZoneMemory();
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown()
{
    // Free allocated zone memory.
    free(mainzone);
    free(smallzone);
}
