

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <SDL/SDL.h>

#include "chrono.h"

typedef struct {
  float x, y, z, speed;
}Stars;

static void StarCreator(Stars* st, int max_stars){
  for(int i = 0; i < max_stars; i++){
    st[i].x = (rand()%2000) - 1000.0f;
    st[i].y = (rand()%2000) - 1000.0f;
    st[i].z = rand()%2000;
    st[i].speed = (rand()%100) + 1.0f;
  }
}

// --------------------------------------------------------------------------
static int cpu_mhz = 0;
static int dump = 0;

static void ChronoShow ( char* name, int computations) {
  float ms = ChronoWatchReset();
  float cycles = ms * (1000000.0f/1000.0f) * (float)cpu_mhz;
  float cyc_per_comp = cycles / (float)computations;
  if ((dump & 15) == 0)
    fprintf ( stdout, "%s: %f ms, %d cycles, %f cycles/loop\n", name, ms, (int)cycles, cyc_per_comp);
}


// // Limit framerate and return any remaining time to the OS 

static void FramerateLimit (int max_fps) {   
  static unsigned int frame_time = 0;

  unsigned int t = GetMsTime();
  unsigned int elapsed = t - frame_time;
  // El tiempo sobrante se "regala" al sistema operativo con la funcion POSIX sleep
  const unsigned int limit = 1000 / max_fps;  // 1000 miliseconds / framerate minimo deseado 
  if ( elapsed < limit)
    usleep (( limit - elapsed) * 1000); // arg in microseconds
  frame_time = GetMsTime();
}
// ---------------------------------------------------------------------------

// Fully UNoptimized graphics effect example
static void DoEffect (unsigned int* drawlist, int w, int h, int pitch, float projection, Stars* st, int maxStars) {
  // Starfield
  for(int i = 0; i < maxStars; i++){
    float xp = 0.0f;
    float yp = 0.0f;
    if(st[i].z > 0){
      xp = (st[i].x * projection) / st[i].z;
      yp = (st[i].y * projection) / st[i].z;
    }


    if(xp <= w && yp <= h && xp >= 0 && yp >= 0){
      drawlist[0] = xp;
      drawlist[1] = yp;
      drawlist[2] = 255;
      drawlist += 3;
    }
 
    st[i].z -= st[i].speed;

    if(st[i].z <= 0){
      st[i].z += 2000.0f;
      st[i].x = (rand()%2000) - 1000.0f;
      st[i].y = (rand()%2000) - 1000.0f;
      st[i].z = rand()%2000;
    }
  }
}

// ---------------------------------------------------------------------------

static void DisplayVertices (unsigned int* pixels, unsigned int* drawlist, int n_vertices, int stride, unsigned int* palette)
{
  int i;
  for(i = 0; i < n_vertices; i++) {
      int xp = drawlist[0];
      int yp = drawlist[1];
      pixels [xp + yp * stride] = palette[drawlist[2]];
      drawlist += 3;
  } 
}


// ---------------------------------------------------------------------------

static unsigned int palette [256];

int main ( int argc, char** argv) {
  srand(time(NULL));
  int end = 0;
  int mouse_x = 0, mouse_y = 0;
  SDL_Surface  *g_SDLSrf;
  int req_w = 1024;
  int req_h = 768; 

  if ( argc < 2) { fprintf ( stderr, "I need the cpu speed in Mhz!\n"); exit(0);}
  cpu_mhz = atoi( argv[1]);
  // assert(cpu_mhz > 0);
  fprintf ( stdout, "Cycle times for a %d Mhz cpu\n", cpu_mhz);

  // Init SDL and screen
  if ( SDL_Init( SDL_INIT_VIDEO) < 0 ) {
      fprintf(stderr, "Can't Initialise SDL: %s\n", SDL_GetError());
      exit(1);
  }
  if (0 == SDL_SetVideoMode( req_w, req_h, 32,  SDL_HWSURFACE | SDL_DOUBLEBUF)) {
      printf("Couldn't set %dx%dx32 video mode: %s\n", req_w, req_h, SDL_GetError());
      return 0;
  }

  // Dot palette
  int i;
  for (i=0; i<256; i++)
    palette [i] = i | (i << 8) | (i << 16);

  // Small footprint buffer for vertices draw  
  // we are not using an structure to avoid padding to 8 bytes (instead of current 6)
  //int n_vertices = 10000;
  //short* drawlist = (short*) malloc (n_vertices * 3 * sizeof(short));

  // Setup your effect initialization here
  // Horizontal field of view
  float hfov = 60.0f * ((3.1416f * 2.0f) / 360.0f);  // Degrees to radians
  float half_scr_w = (float)(req_w >> 1);
  float projection = (1.0f / tan ( hfov * 0.5f)) * half_scr_w;

  //////////////////////////////////////////////////////
  ///////        OUR STUFF              ////////////////
  //////////////////////////////////////////////////////

  unsigned int max_stars = 40000;

  Stars constalation[max_stars];
  StarCreator(constalation, max_stars);

  unsigned int* drawList = (unsigned int*)malloc((max_stars * 3) * sizeof(unsigned int));

  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////


  // Main loop
  g_SDLSrf = SDL_GetVideoSurface();
  while ( !end) { 

    SDL_Event event;

    // Your gfx effect goes here

    ChronoWatchReset();
    DoEffect (drawList, g_SDLSrf->w, g_SDLSrf->h, dump, projection, constalation, max_stars);
    //assert(n_draw <= n_vertices);
    //ChronoShow ( "Donut festival", n_draw);

    // Draw vertices; don't modify this section
    // Lock screen to get access to the memory array
    SDL_LockSurface( g_SDLSrf);

    // Clean the screen
    SDL_FillRect(g_SDLSrf, NULL, SDL_MapRGB(g_SDLSrf->format, 0, 0, 0));
    ChronoShow ( "Clean", g_SDLSrf->w * g_SDLSrf->h);

    // Paint vertices
    DisplayVertices (g_SDLSrf->pixels, drawList, max_stars, g_SDLSrf->pitch >> 2, palette);
    //ChronoShow ( "Preview", n_draw);

    //Unlock the draw surface, dump to physical screen
    ChronoWatchReset();
    SDL_UnlockSurface( g_SDLSrf);
    SDL_Flip( g_SDLSrf);
    ChronoShow ( "Screen dump", g_SDLSrf->w * g_SDLSrf->h);

    // Limit framerate and return any remaining time to the OS
    // Comment this line for benchmarking
    FramerateLimit (60);

    dump++;

    // Recoger eventos de la ventana
    while ( SDL_PollEvent(&event) ) 
    {
      switch (event.type) 
      {
        case SDL_MOUSEMOTION:
          mouse_x = event.motion.x;
          mouse_y = event.motion.y;
          break;
        case SDL_MOUSEBUTTONDOWN:
          //printf("Mouse button %d pressed at (%d,%d)\n",
          //       event.button.button, event.button.x, event.button.y);
          break;
        case SDL_QUIT:
          end = 1;
          break;
      }
    }
  }

  return 1;
}


