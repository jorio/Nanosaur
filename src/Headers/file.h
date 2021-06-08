//
// file.h
//

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/
		
			/* Hedr */
			
typedef struct
{
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	SInt32 				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	TQ3Point3D			coord;						// absolute coord (not relative to parent!) 
	UInt16				numPointsAttachedToBone;	// # vertices/points that this bone has
	UInt16				numNormalsAttachedToBone;	// # vertex normals this bone has
	UInt32				reserved[8];				// reserved for future use
}File_BoneDefinitionType;







			/* AnHd */
			
typedef struct
{
	Str32	animName;			
	short	numAnimEvents;	
}SkeletonFile_AnimHeader_Type;


//=================================================

extern	SkeletonDefType *LoadSkeletonFile(short skeletonType);
void	OpenGameFile(const char* filename, short *fRefNumPtr, const char* errString);
OSErr MakePrefsFSSpec(const char* prefFileName, FSSpec* spec);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
extern	void SavePrefs(PrefsType *prefs);
extern	Ptr	LoadAFile(FSSpec* fsSpec, long* outSize);

extern	void LoadTerrainTileset(FSSpec *fsSpec);
extern	void LoadTerrain(FSSpec *fsSpec);

extern	void LoadLevelArt(short levelNum);



















