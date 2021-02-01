//
// misc.h
//


extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
extern void DoAssert(const char* msg, const char* file, int line);
extern void	DoFatalAlert(const char*);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	Ptr	AllocPtr(long size);
extern	void InitMyRandomSeed(void);
extern	void DoFatalAlert2(const char* s1, const char* s2);
extern	float RandomFloat(void);
extern	unsigned long MyRandomLong_Alt(void);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);

OSErr DrawPictureToScreen(FSSpec *myFSSpec, short x, short y);


#define GAME_ASSERT(condition) do { if (!(condition)) DoAssert(#condition, __func__, __LINE__); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoAssert(message, __func__, __LINE__); } while(0)

#define CHECK_GL_ERROR()												\
	do {					 											\
		GLenum err = glGetError();										\
		GAME_ASSERT_MESSAGE(err == GL_NO_ERROR, (const char*) gluErrorString(err));	\
	} while(0)




