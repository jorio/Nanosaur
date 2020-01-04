//
// file.h
//

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/
		
			/* Hedr */
			
typedef struct
{
	BE<SInt16>	version;			// 0xaa.bb
	BE<SInt16>	numAnims;			// gNumAnims
	BE<SInt16>	numJoints;			// gNumJoints
	BE<SInt16>	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	BE<SInt32> 			parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	BE<TQ3Point3D>		coord;						// absolute coord (not relative to parent!) 
	BE<UInt16>			numPointsAttachedToBone;	// # vertices/points that this bone has
	BE<UInt16>			numNormalsAttachedToBone;	// # vertex normals this bone has
	BE<UInt32>			reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* Joit */
			
typedef struct
{
	BE<TQ3Vector3D>	maxRot;						// max rot values of joint
	BE<TQ3Vector3D>	minRot;						// min rot values of joint
	BE<SInt32> 		parentBone; 				// index to previous link joint definition
	unsigned char	name[32];					// text string name for joint
	BE<SInt32>		limbIndex;					// index into limb list
}Joit_Rez_Type;




			/* AnHd */
			
typedef struct
{
	Str32	animName;			
	BE<SInt16>	numAnimEvents;	
}SkeletonFile_AnimHeader_Type;


//=================================================

extern	SkeletonDefType *LoadSkeletonFile(short skeletonType);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
extern	void SavePrefs(PrefsType *prefs);
extern	void SaveGame(void);
extern	OSErr LoadSavedGame(void);
extern	Ptr	LoadAFile(FSSpec *fsSpec);

extern	void LoadTerrainTileset(FSSpec *fsSpec);
extern	void LoadTerrain(FSSpec *fsSpec);

extern	PicHandle LoadAPict(FSSpec *specPtr);

extern	void LoadLevelArt(short levelNum);



















