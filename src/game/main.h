//
// main.h
//

enum
{
	LEVEL_NUM_0
};

enum
{
	PLAY_MODE_SINGLE,
	PLAY_MODE_GATHER,
	PLAY_MODE_JOIN,
	PLAY_MODE_CLIENT,
	PLAY_MODE_HOST
};

#define	SERVER_PLAYER_NUM		0
  
//=================================================


extern	void main(void);
extern	void ToolBoxInit(void);
extern	void UpdateGame_Server(void);


