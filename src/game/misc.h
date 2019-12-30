//
// misc.h
//


extern	void ShowSystemErr(long err);
extern void	ErrorHandler(short);
extern void	DoAlert(Str255);
extern void	DoFatalAlert(Str255);
extern void	Wait(long);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	float	CalcFramesPerSecond(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	Ptr	AllocPtr(long size);
extern	void PrintFPS(void);
extern	void PStringToC(char *pString, char *cString);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	void DoFatalAlert2(Str255 s1, Str255 s2);
extern	float RandomFloat(void);
extern	unsigned long MyRandomLong_Alt(void);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
extern	void ApplyFrictionToDeltas(float f,TQ3Vector3D *d);

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld);
OSErr DrawPictureToScreen(FSSpec *myFSSpec, short x, short y);







