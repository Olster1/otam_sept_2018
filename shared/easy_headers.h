#include <stdio.h>
#include <time.h> // to init random generator
#include "easy_types.h"
// #include <string.h>


#include "easy.h"

static char* globalExeBasePath;

#include "easy_files.h"
#include "easy_math.h"
#include "easy_error.h"
#include "easy_array.h"
#include "sdl_audio.h"
#include "easy_lex.h"
#include "easy_render.h"
#include "easy_camera.h"

#include "easy_3d.h"
#include "easy_utf8.h"
#include "easy_font.h"
#include "easy_timer.h"

#define GJK_IMPLEMENTATION 
#include "easy_gjk.h"
#include "easy_physics.h"
#include "easy_text_io.h"
//#include "easy_sdl_joystick.h"
#include "easy_perlin.h"
#include "easy_tweaks.h"
#include "easy_particle_effects.h"
#include "easy_os.h"
#include "easy_animation.h"
#include "easy_assets.h"
#include "easy_tile.h"
#include "easy_texture_atlas.h"