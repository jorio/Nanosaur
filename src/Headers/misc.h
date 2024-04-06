//
// misc.h
//

#if _MSC_VER
	#define _Static_assert static_assert
#endif

#if OSXPPC && !_DEBUG
#define GAME_ASSERT(condition)
#define GAME_ASSERT_MESSAGE(condition, message)
#else
#define GAME_ASSERT(condition)											\
	do {																\
		if (!(condition))												\
			DoAssert(#condition, __func__, __LINE__);					\
	} while(0)
#define GAME_ASSERT_MESSAGE(condition, message)							\
	do {																\
		if (!(condition))												\
			DoAssert(message, __func__, __LINE__);						\
	} while(0)
#endif

#define AllocHandle(size) NewHandle(size)
#define AllocHandleClear(size) NewHandleClear(size)
#define AllocPtr(size) NewPtr(size)
#define AllocPtrClear(size) NewPtrClear(size)

POMME_NORETURN void ShowSystemErr(long err);
void	DoAlert(const char*);
POMME_NORETURN void DoAssert(const char* msg, const char* file, int line);
POMME_NORETURN void	DoFatalAlert(const char*);
POMME_NORETURN void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
POMME_NORETURN void DoFatalAlert2(const char* s1, const char* s2);
extern	float RandomFloat(void);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);

OSErr DrawPictureToScreen(FSSpec *myFSSpec, short x, short y);

void DoSettingsScreen(void);

void SetProModeSettings(int isPro);

static inline int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}
