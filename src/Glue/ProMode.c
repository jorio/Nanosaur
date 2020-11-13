// There used to be two separate builds for the game: standard Nanosaur, and
// Nanosaur Extreme. The settings below used to be differentiated with the
// PRO_MODE preprocessor define. However, the source port lets you pick either
// version of the game with the same build.

int		PRO_MODE;
int		SUPERTILE_ACTIVE_RANGE;
int		MAX_ENEMIES;
int		MAX_PTERA;
int		MAX_REX;
int		MAX_SPITTER;
int		MAX_STEGO;
int		MAX_TRICER;
int		EXPLODEGEOMETRY_DENOMINATOR;
float	YON_DISTANCE;
float	SONIC_SCREAM_RATE;
float	BLASTER_RATE;
float	HEATSEEK_RATE;
float	TRIBLAST_RATE;
float	NUKE_RATE;

void SetProModeSettings(int pro)
{
	PRO_MODE					= pro;
	SUPERTILE_ACTIVE_RANGE		= pro ? 4 : 3;
	MAX_ENEMIES					= pro ? 30 : 8;
	MAX_PTERA					= pro ? 10 : 2;
	MAX_REX						= pro ? 8 : 2;
	MAX_SPITTER					= pro ? 2 : 12;  // weird... but these values were in the original source
	MAX_STEGO					= pro ? 10 : 2;
	MAX_TRICER					= pro ? 10 : 3;
	EXPLODEGEOMETRY_DENOMINATOR	= pro ? 1 : 4;
	YON_DISTANCE				= pro ? 2800.0f : 1900.0f;
	SONIC_SCREAM_RATE			= pro ? 4 : 4;
	BLASTER_RATE				= pro ? 7 : 4;
	HEATSEEK_RATE				= pro ? 2 : 2;
	TRIBLAST_RATE				= pro ? 7 : 3;
	NUKE_RATE					= pro ? .05 : .05;
}
