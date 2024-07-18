#ifndef __LT_CODES__
#define __LT_CODES__

#define LTFALSE		0
#define LTTRUE		1

#define CWO_FLG_ADDED		(1<<0)
#define CWO_FLG_REMOVED		(1<<1)

#define FILE_ANYFILE	0
#define FILE_CLIENTFILE	1
#define FILE_SERVERFILE	2

#define TYPECODE_WORLD		0
#define TYPECODE_MODEL		1
#define TYPECODE_SPRITE		2
#define TYPECODE_TEXTURE	3
#define TYPECODE_SOUND		4
#define TYPECODE_DLL		5
#define TYPECODE_UNKNOWN    0xFF

#define OT_NORMAL			0
#define OT_MODEL			1
#define OT_WORLDMODEL		2
#define OT_SPRITE			3
#define OT_LIGHT			4
#define OT_CAMERA			5
#define OT_PARTICLESYSTEM	6 
#define OT_POLYGRID			7
#define OT_LINESYSTEM		8
#define OT_CONTAINER		9
#define OT_CANVAS			10

#define NUM_OBJECT_TYPES	11

#define CF_NOTIFYREMOVE		(1<<0)
#define CF_NOTIFYMODELKEYS	(1<<1)
#define CF_DONTSETDIMS		(1<<2)
#define CF_INSIDERADIUS		(1<<3)
#define CF_SOLIDCANVAS		(1<<4)

#define GLOBALPAN_SKYSHADOW		0
#define GLOBALPAN_FOGLAYER		1
#define NUM_GLOBALPAN_TYPES		2

#define MAX_DTX_MIPMAPS			8
#define DTX_COMMANDSTRING_LEN	128
#define DTX_FULLBRITE	(1<<0)

#define CLEARSCREEN_SCREEN	1
#define CLEARSCREEN_RENDER	2

#define DRAWMODE_NORMAL     1
#define DRAWMODE_OBJECTLIST	2

#define MHF_USETEXTURE	1

#define BLIT_TRANSPARENT	1

#define INVALID_HPOLY	0xFFFFFFFF

#define SC_PLAY		(1<<0)
#define SC_LOOP		(1<<1)

#define MAX_MODEL_TEXTURES	4
#define MAX_PIECES_PER_MODEL	32

#define MIH_CLIPPLANE	(1<<0)

#define MAX_PIECENAME_LEN	32
#define MAX_SOCKETNAME_LEN		16
#define MAX_WEIGHTSETNAME_LEN	16

#define MNODE_ROTATIONONLY		(1<<1)
#define NODEPARENT_NONE			0xFFFFFFFF
#define MODELFLAG_CACHED		(1<<0)
#define MODELFLAG_CACHED_CLIENT	(1<<1)

#define KEYTYPE_POSITION	0
#define KEYTYPE_CALLBACK	1

#define MAX_CHILD_MODELS	16

#define WD_ORIGINALBSPALLOCED	(1<<0) // incorrect? 1 - is VisBSP?
#define WD_WORLDBSPALLOCED		(1<<1) // incorrect?

#define WIF_UNK0				(1<<0) 
#define WIF_MOVEABLE            (1<<1) // ?
#define WIF_MAINWORLD           (1<<2)
#define WIF_TERRAIN				(1<<3) // ?
#define WIF_PHYSICSBSP          (1<<4)
#define WIF_VISBSP              (1<<5)
#define WIF_UNK6				(1<<6) // ?
#define WIF_UNK7				(1<<7) // ?

#define MAX_LIGHTANIMNAME_LEN	32
#define INVALID_LIGHT_ANIM		0xFFFFFFFF
#define LIGHTANIMFRAME_NONE		0xFFFFFFFF

#define MAX_WORLDNAME_LEN	64

#define ST_TAGGED           0x8000
#define ST_VALIDTEXTUREINFO	0x4000
#define ST_REFCOUNTMASK     0x3FFF

#define POLY_GRID_COLOR_TABLE_SIZE	256

#define MODELFLAG_TOUCHED	(1<<0)

#define MAX_NODE_LEVEL	16

#define LTRENDER_VERSION	3421
#define RENDER_OK			0
#define RENDER_ERROR		1

#define LT_OK	0
#define LT_ERROR	1
#define LT_FINISHED	2

#define MATH_DEGREES_TO_RADIANS(x) ((x) *  0.01745329251994f)
#define MATH_ONE_OVER_255   0.003921568627451f

#define INLINE_FN __inline

template<class T, class TB>
INLINE_FN T LTDIFF(T a, TB b) { return ((a < (T)b) ? ((T)b - a) : (a - (T)b)); }
template<class T, class TB>
INLINE_FN T LTMIN(T a, TB b) { return ((a < (T)b) ? a : (T)b); }
template<class T, class TB>
INLINE_FN T LTMAX(T a, TB b) { return ((a > (T)b) ? a : (T)b); }
template<class T>
INLINE_FN T LTABS(T a) { return ((a >= 0) ? a : -a); }
template<class T, class TB, class TC>
INLINE_FN T LTCLAMP(T a, TB min, TC max) { return ((a < (T)min) ? (T)min : ((a > (T)max) ? (T)max : a)); }
template<class T, class TMAX, class TINTERP>
INLINE_FN T LTLERP(T min, TMAX max, TINTERP t) { return (min + (((T)max - min) * t)); }

#define FLAG_VISIBLE				(1<<0)
#define FLAG_SHADOW					(1<<1)
#define FLAG_UNSIGNED				(1<<1)
#define FLAG_SPRITEBIAS				(1<<2)
#define FLAG_MODELTINT				(1<<2)
#define FLAG_CASTSHADOWS			(1<<3)
#define FLAG_ROTATEABLESPRITE		(1<<3)
#define FLAG_DETAILTEXTURE			(1<<3)
#define FLAG_UPDATEUNSEEN			(1<<3)
#define FLAG_SOLIDLIGHT				(1<<4)
#define FLAG_MODELWIREFRAME			(1<<4)
#define FLAG_WASDRAWN				(1<<4)
#define FLAG_SPRITE_NOZ				(1<<4)
#define FLAG_GLOWSPRITE				(1<<5)
#define FLAG_ONLYLIGHTWORLD			(1<<5)
#define FLAG_ENVIRONMENTMAP			(1<<5)
#define FLAG_ENVIRONMENTMAPONLY		(1<<5)
#define FLAG_DONTLIGHTBACKFACING	(1<<6)
#define FLAG_REALLYCLOSE			(1<<6)
#define FLAG_FOGDISABLE				(1<<7)
#define FLAG_ANIMTRANSITION			(1<<7)
#define FLAG_ONLYLIGHTOBJECTS		(1<<7)
#define FLAG_FULLPOSITIONRES		(1<<8)
#define FLAG_NOLIGHT				(1<<9)
#define FLAG_HARDWAREONLY			(1<<10)
#define FLAG_PORTALVISIBLE			(1<<10)
#define FLAG_YROTATION				(1<<11)
#define FLAG_RAYHIT					(1<<12)
#define FLAG_SOLID					(1<<13)
#define FLAG_BOXPHYSICS				(1<<14)
#define FLAG_NOGLOBALLIGHTSCALE		(1<<14)
#define FLAG_CLIENTNONSOLID			(1<<15)
#define FLAG_TOUCH_NOTIFY			(1<<16)
#define FLAG_GRAVITY				(1<<17)
#define FLAG_STAIRSTEP				(1<<18)
#define FLAG_MODELKEYS				(1<<19)
#define FLAG_KEEPALIVE				(1<<20)
#define FLAG_GOTHRUWORLD			(1<<21)
#define FLAG_DONTFOLLOWSTANDING		(1<<23)
#define FLAG_FORCECLIENTUPDATE		(1<<24)
#define FLAG_NOSLIDING				(1<<25)
#define FLAG_POINTCOLLIDE			(1<<26)
#define FLAG_REMOVEIFOUTSIDE		(1<<27)
#define FLAG_FORCEOPTIMIZEOBJECT	(1<<28)
#define	FLAG_CONTAINER				(1<<29)
#define FLAG_INTERNAL1				(1<<30)
#define FLAG_INTERNAL2				(1<<31)
#define FLAG_LASTFLAG				FLAG_INTERNAL2

#define FLAG2_PORTALINVISIBLE		(1<<0)
#define FLAG2_CHROMAKEY				(1<<1)
#define FLAG2_ADDITIVE				(1<<2)
#define FLAG2_MULTIPLY				(1<<3)
#define FLAG2_CYLINDERPHYSICS		(1<<4)
#define FLAG2_SPHEREPHYSICS			(1<<5)
#define FLAG2_ORIENTMOVEMENT		(1<<6)
#define FLAG2_DYNAMICDIRLIGHT		(1<<7)
#define FLAG2_SKYOBJECT				(1<<8)
#define FLAG2_SPRITE_TROTATE		(1<<9)
#define FLAG2_SEMISOLID				(1<<10)
#define FLAG2_LASTFLAG				FLAG2_SEMISOLID

#define INTERSECT_OBJECTS	1
#define IGNORE_NONSOLID		2
#define INTERSECT_HPOLY		4

// TODO - name is unknown
#define IFLAG_IGNORE_VIS_QUERY		0x400

#endif
