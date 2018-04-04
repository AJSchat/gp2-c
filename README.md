# gpc-2
gp2-c is a pure C implementation of Raven's Generic Parser 2, commonly referred
to as GP2. Next to porting the core features, there have been some slight API
changes in light of either security or unused functionality (i.e. variables
that lead to the exact same end result).

GP2 was originally developed by Raven Software in C++. You can find both the
original C++ code and the new C code in this repository.

This implementation only focuses on including the C interface, that was already
available in the original C++ code. The extra functionality, such as writing
GP2 files or duplicating GP2 groups are omitted due to not being referenced
anywhere in the original SoF2 engine or its modules. They may still be added
later on.

### API
Below is the new API for the C implementation of GP2:

```c
// CGenericParser2 (void *) routines.
//=============================================
TGenericParser2     GP_Parse                ( char **dataPtr );
TGenericParser2     GP_ParseFile            ( char *fileName );
void                GP_Clean                ( TGenericParser2 GP2 );
void                GP_Delete               ( TGenericParser2 *GP2 );
TGPGroup            GP_GetBaseParseGroup    ( TGenericParser2 GP2 );

// CGPGroup (void *) routines.
//=============================================
qboolean            GPG_GetName             ( TGPGroup GPG, char *dest, int destSize );
TGPGroup            GPG_GetNext             ( TGPGroup GPG );
TGPGroup            GPG_GetInOrderNext      ( TGPGroup GPG );
TGPGroup            GPG_GetInOrderPrevious  ( TGPGroup GPG );
TGPGroup            GPG_GetPairs            ( TGPGroup GPG );
TGPGroup            GPG_GetInOrderPairs     ( TGPGroup GPG );
TGPGroup            GPG_GetSubGroups        ( TGPGroup GPG );
TGPGroup            GPG_GetInOrderSubGroups ( TGPGroup GPG );
TGPGroup            GPG_FindSubGroup        ( TGPGroup GPG, const char *name );
TGPValue            GPG_FindPair            ( TGPGroup GPG, const char *key );
void                GPG_FindPairValue       ( TGPGroup GPG, const char *key, const char *defaultVal, char *dest, int destSize );

// CGPValue (void *) routines.
//=============================================
qboolean            GPV_GetName             ( TGPValue GPV, char *dest, int destSize );
TGPValue            GPV_GetNext             ( TGPValue GPV );
TGPValue            GPV_GetInOrderNext      ( TGPValue GPV );
TGPValue            GPV_GetInOrderPrevious  ( TGPValue GPV );
qboolean            GPV_IsList              ( TGPValue GPV );
qboolean            GPV_GetTopValue         ( TGPValue GPV, char *dest, int destSize );
TGPValue            GPV_GetList             ( TGPValue GPV );
```

The following routines were altered:

* **GP_Parse**: The new implemention expects one parameter. The cleanFirst
parameter was removed due to the routine always returning a new instance. The
writeable parameter is no longer present due to omitting the write GP2 routines.
* **GPG_GetName**, **GPG_FindPairValue**, **GPV_GetName** and
**GPV_GetTopValue**: The overloaded const char* routines were removed.
The qboolean routines now expect a destination buffer size in the destSize
parameter to avoid buffer overflows.

The following routine was added:
* **GP_ParseFile**: Even though not present in the original C++ implementation,
this routine was added due to it also being present in the SoF2 codebase by
Raven Software. Like GP_Parse, the cleanFirst and writeable parameters are
omitted.

### Testing GP2
Included in the repository are a set of test files. They can be tested by
building the test programs for both the new C implementation and the original
C++ implementation.

GNU Make is required in order to build the test programs with the included
Makefile. Only relevant information is shown in the output below.

```bash
$ make
cc -o gp2-test-c obj/c/gp2-c/genericparser2.o obj/c/qcommon/common.o obj/c/qcommon/q_math.o obj/c/qcommon/q_shared.o obj/c/test.o obj/c/main.o -lm
g++ -o gp2-test-cpp obj/cpp/gp2-cpp/genericparser2.o obj/cpp/qcommon/common.o obj/cpp/qcommon/q_math.o obj/cpp/qcommon/q_shared.o obj/cpp/test.o obj/cpp/main.o -lm
$ ls -l
gp2-test-c
gp2-test-cpp
```

Optionally, you can just build the test program for your desired implementation.

The new C GP2 implementation:

```bash
$ make gp2-test-c
```

The original C++ GP2 implementation:

```bash
$ make gp2-test-cpp
```

### License
gp2-c is licensed under the GNU General Public License v3.0. You can find a
copy of the license in the LICENSE.md file in the repository root.
