//
// sprites.h
//

void InitSpriteManager(void);
void LoadSpriteGroup(const char* groupName, short groupNum, int numFrames);
void DisposeSpriteGroup(short groupNum);
void DrawSpriteFrameToScreen(short group, int frame, int x, int y);

