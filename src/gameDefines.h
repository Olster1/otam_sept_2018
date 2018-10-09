#define APP_TITLE "Fitris"
#define RESOURCE_PATH_EXTENSION "../res/" //   ../res/ for developer mode res/ for release bundle
#define MOVE_INTERVAL 1.0f
#define FADE_TIMER_INTERVAL 0.3f
#define SCENE_TRANSITION_TIME 0.3f
#define SCENE_MUSIC_TRANSITION_TIME (SCENE_TRANSITION_TIME + 0.2f) //extra lee way so they overlap 
#define CHEAT_MODE 0
#define START_MENU_MODE PLAY_MODE
#define XP_PER_LINE 100
#define GO_TO_NEXT_GROUP_AUTO 0
#define CAN_ALTER_SHAPE_DIAGONAL 0 //this is if you can move a block to a position only situated diagonally 
#define OPENGL_BACKEND 1
#define RENDER_BACKEND OPENGL_BACKEND
#define RENDER_HANDNESS -1 //Right hand handess -> z going into the screen. 

/***************************
	-> fix windmills
	-> time for waiting - use for loop for windmill
	-> flood fill - check if the islands are the same size
	-> texture atlas 
	-> bombs can be on windmills 
	-> make settings ui
****************************/