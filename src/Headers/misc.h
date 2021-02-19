//
// misc.h
//

#if _MSC_VER
	#define _Static_assert static_assert
#endif

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

extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
extern void DoAssert(const char* msg, const char* file, int line);
extern void	DoFatalAlert(const char*);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	Ptr	AllocPtr(long size);
extern	void DoFatalAlert2(const char* s1, const char* s2);
extern	float RandomFloat(void);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);

OSErr DrawPictureToScreen(FSSpec *myFSSpec, short x, short y);

void DoSettingsScreen(void);

void SetProModeSettings(int isPro);
