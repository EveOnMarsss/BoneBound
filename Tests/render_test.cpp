#include "project.h"
#include "World/WorldMap.h"
#include "World/LocalMap.h"
#include "World/Tiles.h"
#include "Entities/NPC.h"
#include <cstdio>
extern "C" { int GetScreenWidth(void){return 1280;} int GetScreenHeight(void){return 720;} double GetTime(void){return 0;} float GetFrameTime(void){return .016f;} int GetRandomValue(int a,int b){return a+rand()%(b-a+1);} }
// dumps cells as: glyph fg(3) bg(4)
static void Dump(FILE* f,int glyph,Color fg,Color bg){ unsigned char b[8]={(unsigned char)glyph,fg.r,fg.g,fg.b,bg.r,bg.g,bg.b,bg.a}; fwrite(b,1,8,f); }
int main(int argc,char**argv){
  unsigned seed=atoi(argv[1]); int size=atoi(argv[2]); const char* mode=argv[3]; const char* out=argv[4];
  worldSeed=seed; memset(&player,0,sizeof player); strcpy(player.name,"Biscuit"); player.isCat=true;
  GenerateWorldMap(size,size);
  FILE* f=fopen(out,"wb");
  if(!strcmp(mode,"world")){ int w=currentMapWidth,h=currentMapHeight; fwrite(&w,4,1,f); fwrite(&h,4,1,f);
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){ int g;Color fg,bg; WorldTileLook(&worldMap[y][x],&g,&fg,&bg); if(x==player.worldPos.x&&y==player.worldPos.y){g='@';fg=WHITE;bg=BLACK;} Dump(f,g,fg,bg);} }
  else { // local view around world tile (argv[5],argv[6]) centred on cell (argv[7],argv[8]); size argv[9] x argv[10]
    int tx=atoi(argv[5]),ty=atoi(argv[6]),cx=atoi(argv[7]),cy=atoi(argv[8]),vw=atoi(argv[9]),vh=atoi(argv[10]);
    if(tx==-2){ // find: ty = biome id, or 100 = river crossing a road, 101 = coast with river mouth
      bool found=false; for(int y=3;y<size-3&&!found;y++)for(int x=3;x<size-3&&!found;x++){ WorldTile*t=&worldMap[y][x];
        bool ok = (ty==100)? (t->riverMask&&t->roadMask&&!t->isLake&&t->biome>BIOME_LAKE) : (ty==101)? (t->biome==BIOME_BEACH&&t->riverMask) : (t->biome==ty&&t->locationType==0&&(x+y)%3==0);
        if(ok){tx=x;ty=y;found=true;} }
      if(!found){printf("not found\n");return 1;} printf("tile (%d,%d) %s\n",tx,ty,worldMap[ty][tx].locationName); }
    if(tx<0){tx=player.worldPos.x;ty=player.worldPos.y;}
    // generate neighbours too so the view can cross tile edges
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++) if(tx+dx>=0&&ty+dy>=0&&tx+dx<size&&ty+dy<size) GenerateLocalMap(tx+dx,ty+dy);
    fwrite(&vw,4,1,f); fwrite(&vh,4,1,f);
    for(int j=0;j<vh;j++)for(int i=0;i<vw;i++){ int gx=tx*256+cx-vw/2+i, gy=ty*256+cy-vh/2+j; int wx=gx>>8, wy=gy>>8; int lx=gx&255, ly=gy&255;
      if(wx<0||wy<0||wx>=size||wy>=size){Dump(f,' ',BLACK,BLACK);continue;} LocalMap*m=worldMap[wy][wx].localMap; char t=m->tiles[ly][lx]; int b=m->biome[ly*256+lx]; unsigned h=HashCell(gx,gy,1337u);
      Color fg=TileColor(t,b,h), bg=TileBackground(t,b,h); int g=TileGlyph(t,h);
      if(i==vw/2&&j==vh/2){g='@';fg=(Color){255,190,70,255};bg=BLACK;} Dump(f,g,fg,bg);} }
  fclose(f);
}
