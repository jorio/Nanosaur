//
// misc.h
//


extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
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







