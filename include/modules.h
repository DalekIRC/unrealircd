/************************************************************************
 *   Unreal Internet Relay Chat Daemon, include/modules.h
 *   (C) Carsten V. Munk 2000 <stskeeps@tspre.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id$
 */
#ifndef MODULES_H
#define MODULES_H
#define MOD_VERSION	"3.2-b5-1"
#define MOD_WE_SUPPORT  "3.2-b5*"
#define MAXHOOKTYPES	20
typedef void			(*vFP)();	/* Void function pointer */
typedef int			(*iFP)();	/* Integer function pointer */
typedef char			(*cFP)();	/* char * function pointer */
#if defined(_WIN32)
#define DLLFUNC	_declspec(dllexport)
#define irc_dlopen(x,y) LoadLibrary(x)
#define irc_dlclose FreeLibrary
#define irc_dlsym(x,y,z) z = (void *)GetProcAddress(x,y)
#undef irc_dlerror
#elif defined(HPUX)
#define irc_dlopen(x,y) shl_load(x,y,0L)
#define irc_dlsym(x,y,z) shl_findsym(x,y,z)
#define irc_dlclose shl_unload
#define irc_dlerror() strerror(errno)
#else
#define irc_dlopen dlopen
#define irc_dlclose dlclose
#define irc_dlsym(x,y,z) z = dlsym(x,y)
#define irc_dlerror dlerror
#define DLLFUNC 
#endif

#define EVENT(x) void (x) (void *data)

typedef struct _mod_symboltable Mod_SymbolDepTable;
typedef struct _event Event;
typedef struct _eventinfo EventInfo;
typedef struct _irchook Hook;

/*
 * Module header that every module must include, with the name of
 * mod_header
*/

typedef struct _ModuleHeader {
	char	*name;
	char	*version;
	char	*description;
	char	*modversion;
	Mod_SymbolDepTable *symdep;
} ModuleHeader;

/*
 * One piece of Borg ass..
*/
typedef struct _Module Module;

typedef struct _ModuleChild
{
	struct _ModuleChild *prev, *next; 
	Module *child; /* Aww. aint it cute? */
} ModuleChild;

#define MOBJ_EVENT   0x0001
#define MOBJ_HOOK    0x0002
#define MOBJ_COMMAND 0x0004

typedef struct _ModuleObject {
	struct _ModuleObject *prev, *next;
	short type;
	union {
		Event *event;
		Hook *hook;
		aCommand *command;
	} object;
} ModuleObject;

struct _irchook {
	Hook *prev, *next;
	short type;
	union {
		int (*intfunc)();
		void (*voidfunc)();
	} func;
	Module *owner;
};

/*
 * What we use to keep track internally of the modules
*/

struct _Module
{
	struct _Module *prev, *next;
	ModuleHeader    *header; /* The module's header */
#ifdef _WIN32
	HMODULE dll;		/* Return value of LoadLibrary */
#elif defined(HPUX)
	shl_t	dll;
#else
	void	*dll;		/* Return value of dlopen */
#endif	
	unsigned char flags;    /* 8-bits for flags .. */
	ModuleChild *children;
	ModuleObject *objects;
};
/*
 * Symbol table
*/

struct _mod_symboltable
{
#ifndef STATIC_LINKING
	char	*symbol;
#else
	void	*realfunc;
#endif
	vFP 	*pointer;
#ifndef STATIC_LINKING
	char	*module;
#endif
};

#ifndef STATIC_LINKING
#define MOD_Dep(name, container,module) {#name, (vFP *) &container, module}
#else
#define MOD_Dep(name, container,module) {(void *)&name, (vFP *) &container}
#endif
/* Event structs */
struct _event {
	Event   *prev, *next;
	char    *name;
	time_t  every;
	long    howmany;
	vFP	event;
	void    *data;
	time_t  last;
	Module *owner;
};

#define EMOD_EVERY 0x0001
#define EMOD_HOWMANY 0x0002
#define EMOD_NAME 0x0004
#define EMOD_EVENT 0x0008
#define EMOD_DATA 0x0010

struct _eventinfo {
	int flags;
	long howmany;
	time_t every;
	char *name;
	vFP event;
	void *data;
};

#define EventAdd(name, every, howmany, event, data) EventAddEx(NULL, name, every, howmany, event, data)
Event   *EventAddEx(Module *, char *name, long every, long howmany,
                  vFP event, void *data);
Event   *EventDel(Event *event);
Event   *EventFind(char *name);
int     EventMod(Event *event, EventInfo *mods);
void    DoEvents(void);
void    EventStatus(aClient *sptr);
void    SetupEvents(void);


extern Hook		*Hooks[MAXHOOKTYPES];
extern Hook		*global_i;

void    Module_Init(void);
char    *Module_Load(char *path, int load);
int     Module_Unload(char *name, int unload);
vFP     Module_Sym(char *name);
vFP     Module_SymX(char *name, Module **mptr);
int	Module_free(Module *mod);

#define add_Hook(hooktype, func) HookAddMain(NULL, hooktype, func, NULL)
#define HookAdd(hooktype, func) HookAddMain(NULL, hooktype, func, NULL)
#define HookAddEx(module, hooktype, func) HookAddMain(module, hooktype, func, NULL)
#define HookAddVoid(hooktype, func) HookAddMain(NULL, hooktype, NULL, func)
#define HookAddVoidEx(module, hooktype, func) HookAddMain(module, hooktype, NULL, func)
#define add_HookX(hooktype, func1, func2) HookAddMain(NULL, hooktype, func1, func2)

Hook	*HookAddMain(Module *module, int hooktype, int (*intfunc)(), void (*voidfunc)());
Hook	*HookDel(Hook *hook);

#define RunHook0(hooktype) for (global_i = Hooks[hooktype]; global_i; global_i = global_i->next)(*(global_i->func.intfunc))()
#define RunHook(hooktype,x) for (global_i = Hooks[hooktype]; global_i; global_i = global_i->next) (*(global_i->func.intfunc))(x)
#define RunHookReturn(hooktype,x,ret) for (global_i = Hooks[hooktype]; global_i; global_i = global_i->next) if((*(global_i->func.intfunc))(x) ret) return -1
#define RunHook2(hooktype,x,y) for (global_i = Hooks[hooktype]; global_i; global_i = global_i->next) (*(global_i->func.intfunc))(x,y)


/* Hook types */
#define HOOKTYPE_LOCAL_QUIT	1
#define HOOKTYPE_LOCAL_NICKCHANGE 2
#define HOOKTYPE_LOCAL_CONNECT 3
#define HOOKTYPE_SCAN_HOST 4	/* Taken care of in scan.c */
#define HOOKTYPE_SCAN_INFO 5    /* Taken care of in scan.c */
#define HOOKTYPE_CONFIG_UNKNOWN 6
#define HOOKTYPE_REHASH 7
#define HOOKTYPE_PRE_LOCAL_CONNECT 8
#define HOOKTYPE_HTTPD_URL 9
#define HOOKTYPE_GUEST 10
#define HOOKTYPE_SERVER_CONNECT 11
#define HOOKTYPE_SERVER_QUIT 12

/* Module flags */
#define MODFLAG_NONE	0x0000
#define MODFLAG_LOADED	0x0001 /* (mod_load has been called and suceeded) */

/* Module function return values */
#define MOD_SUCCESS 0
#define MOD_FAILED -1
#define MOD_DELAY 2
#endif
