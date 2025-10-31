#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct { int x, y, w, h; } Rect;

static inline Uint32 getpx(SDL_Surface* s, int x, int y){
  Uint8* row = (Uint8*)s->pixels + y * s->pitch;
  return ((Uint32*)row)[x];
}
static inline void setpx(SDL_Surface* s, int x, int y, Uint32 v){
  Uint8* row = (Uint8*)s->pixels + y * s->pitch;
  ((Uint32*)row)[x] = v;
}
static inline void draw_rect(SDL_Surface* img, Rect r, Uint32 color){
  if (r.w<=0||r.h<=0) return;
  int x0=r.x, y0=r.y, x1=r.x+r.w-1, y1=r.y+r.h-1;
  if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
  for (int x=x0; x<=x1; x++){ setpx(img,x,y0,color); setpx(img,x,y1,color); }
  for (int y=y0; y<=y1; y++){ setpx(img,x0,y,color); setpx(img,x1,y,color); }
  if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
}
static inline Rect clamp(SDL_Surface* s, Rect r){
  if (r.x<0) r.x=0; if (r.y<0) r.y=0;
  if (r.x+r.w>s->w) r.w = s->w - r.x;
  if (r.y+r.h>s->h) r.h = s->h - r.y;
  if (r.w<0) r.w=0; if (r.h<0) r.h=0; return r;
}

int main(int argc, char** argv){
  if (argc < 5){ fprintf(stderr,"usage: %s rotated.bmp grid_roi.txt ROWS COLS\n", argv[0]); return 1; }
  int R = atoi(argv[3]), C = atoi(argv[4]); if (R<2||C<2){ fprintf(stderr,"ROWS/COLS must be >=2\n"); return 1; }
  if (SDL_Init(0)!=0){ fprintf(stderr,"SDL_Init: %s\n", SDL_GetError()); return 1; }
  if (!(IMG_Init(IMG_INIT_PNG))){ fprintf(stderr,"IMG_Init: %s\n", IMG_GetError()); SDL_Quit(); return 1; }

  SDL_Surface* src = IMG_Load(argv[1]); if(!src){ fprintf(stderr,"load fail: %s\n", IMG_GetError()); return 1; }
  Rect grid; FILE* fr=fopen(argv[2],"r"); if(!fr||fscanf(fr,"%d %d %d %d",&grid.x,&grid.y,&grid.w,&grid.h)!=4){ fprintf(stderr,"bad grid_roi.txt\n"); return 1; }
  if(fr) fclose(fr); grid = clamp(src, grid);

  int cellW = grid.w / C, cellH = grid.h / R;
  FILE* csv=fopen("cells.csv","w"); fprintf(csv,"row,col,x,y,w,h\n");

  SDL_Surface* ov = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGB888, 0);
  Uint32 red = SDL_MapRGB(ov->format,255,0,0);

  // inset to avoid cell borders
  int insetX = (int)(cellW * 0.08), insetY = (int)(cellH * 0.08);

  system("mkdir -p output/letters >/dev/null 2>&1");

  for(int r=0;r<R;r++){
    for(int c=0;c<C;c++){
      int x0 = grid.x + c*cellW;
      int y0 = grid.y + r*cellH;
      Rect cell = (Rect){ x0, y0, cellW, cellH };
      Rect crop = (Rect){ x0+insetX, y0+insetY, cellW-2*insetX, cellH-2*insetY };
      crop = clamp(src, crop);
      fprintf(csv,"%d,%d,%d,%d,%d,%d\n", r,c,crop.x,crop.y,crop.w,crop.h);
      draw_rect(ov, cell, red);

      SDL_Rect rc = {crop.x, crop.y, crop.w, crop.h};
      SDL_Surface* letter = SDL_CreateRGBSurfaceWithFormat(0, rc.w, rc.h, 32, src->format->format);
      SDL_BlitSurface(src, &rc, letter, NULL);
      char path[128]; snprintf(path,sizeof(path),"output/letters/%02d_%02d.png", r,c);
      IMG_SavePNG(letter, path);
      SDL_FreeSurface(letter);
    }
  }
  fclose(csv);
  SDL_SaveBMP(ov,"stage_cells.bmp");

  SDL_FreeSurface(ov); SDL_FreeSurface(src); IMG_Quit(); SDL_Quit();
  fprintf(stderr,"Saved stage_cells.bmp, cells.csv, output/letters/*\n");
  return 0;
}
