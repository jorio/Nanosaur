//
// sprites.h
//

#define	MAX_SHAPE_ANIMS		50


typedef struct 
{
	short		opcode;
	short		operand;
}AnimEntryType;


		/* SHAPE TABLE STRUCTS */
		
typedef struct
{
	short			width;
	short			height;
	short			xoffset;
	short			yoffset;
	Boolean			hasMask;
	u_short			**pixelData;			// pointer to pixel & mask data
}ShapeFrameHeader;


typedef struct
{
	short			numLines;				// # lines in anim
	AnimEntryType	**animData;				// handle to anim data
}ShapeAnimHeader;


typedef struct
{
	u_short				numFrames;		// # frames in this shape table/frames file
	ShapeFrameHeader	**frameHeaders;

	u_short				numAnims;		// # anims in this shape table/frames file
	ShapeAnimHeader		anims[MAX_SHAPE_ANIMS];
}ShapeTableHeader;


		/* FRAMES FILE STRUCTS */
		
typedef struct
{
	u_short		version;
	u_short		numFrames;
	u_short		numAnims;
	RGBColor	xColor;
}FramesFile_Header_Type;




struct FrameHeaderType
{
	long			width;
	long			height;
	long			xoffset;
	long			yoffset;
	Boolean			hasMask;
};
typedef struct FrameHeaderType FrameHeaderType;


struct AnimHeaderType
{
	u_short			numLines;
};
typedef struct AnimHeaderType AnimHeaderType;



//=====================================================


extern	void InitSpriteManager(void);
extern	void LoadFramesFile(FSSpec *inFile, short groupNum);
extern	void DisposeSpriteGroup(short groupNum);
extern	void DrawSpriteFrameToScreen(u_long group, u_long frame, long x, long y);












