#define DEVELOPER_MODE 1
#define DEMO_MODE 0
#define APP_TITLE "Feoh the Fitter"
#define MOVE_INTERVAL 1.0f
#define FADE_TIMER_INTERVAL 0.3f
#define SCENE_TRANSITION_TIME 0.3f
#define SCENE_MUSIC_TRANSITION_TIME (SCENE_TRANSITION_TIME + 0.2f) //extra lee way so they overlap 
#define CHEAT_MODE 1
#define START_MENU_MODE MENU_MODE
#define XP_PER_LINE 100
#define GO_TO_NEXT_GROUP_AUTO 0
#define UI_BUTTON_COLOR COLOR_YELLOW
#define CAN_ALTER_SHAPE_DIAGONAL 0 //this is if you can move a block to a position only situated diagonally 
#define OPENGL_BACKEND 1
#define RENDER_BACKEND OPENGL_BACKEND
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 1
#define RENDER_HANDNESS -1 //Right hand handess -> z going into the screen. 

/***************************
 -> more to explore: 
     1. choosing which bombs to blow up based on health bar 
     2. joining up the shape when there is a hole
     3. explore ledges more
     5. do more with windmill shapes, just like a fast pace action game
     4. two shapes really far apart, have to manage them in conjunction
     5. mirror levels 
     
 -> fix font bug for writing the levels - maybe make a font atlas - y is sometimes missing
 -> fix malloc bug 
 -> have 'Unity' settings box at start of game for full screen 
 
 -> freestyle arena 
 -> make sure you can beat all the levels. 
 -> level editor for people
 
 -> itch.io webpage  (~1 day work) -> just itch.io
 
 -> Video Footage (~1 day)
 
 -> Windows build (~1 day work)
 
 extended:
 Android build with SDL (~ 2 days work)
 Put on steam (~2 days work)
 
 
 
****************************/