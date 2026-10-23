#include "World/WorldGen.h"
#include "World/LocationNames.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
static void Col(BiomeType b, unsigned char* c){
  static const unsigned char T[][3]={{15,30,95},{35,75,160},{70,140,225},{235,225,150},{215,185,100},{185,170,70},{100,175,60},{40,120,40},{20,150,50},{85,105,55},{60,110,100},{150,170,175},{225,240,255},{150,125,80},{125,115,105},{250,250,255}};
  memcpy(c,T[b],3);
}
int main(int argc,char**argv){
  int W=atoi(argv[1]); unsigned seed=atoi(argv[2]); const char* out=argv[3];
  WorldTile** m=(WorldTile**)malloc(W*sizeof(WorldTile*)); for(int y=0;y<W;y++) m[y]=(WorldTile*)calloc(W,sizeof(WorldTile));
  auto t0=std::chrono::steady_clock::now(); int sx,sy;
  WorldGen_Generate(W,W,seed,m,&sx,&sy);
  double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
  int cnt[BIOME_COUNT]={0}, land=0, rivers=0, roads=0, lakes=0, sites[9]={0};
  for(int y=0;y<W;y++)for(int x=0;x<W;x++){WorldTile*t=&m[y][x];cnt[t->biome]++; if(t->biome>BIOME_LAKE)land++; if(t->riverMask&&!t->isLake)rivers++; if(t->roadMask)roads++; if(t->isLake)lakes++; sites[t->locationType]++;}
  printf("size %d seed %u: %.0f ms | land %.0f%% | rivertiles %d lakes %d roadtiles %d | start (%d,%d) '%s'\n",W,seed,ms,100.0*land/(W*W),rivers,lakes,roads,sx,sy,m[sy][sx].locationName);
  const char* bn[]={"deepocean","ocean","lake","beach","desert","savanna","grass","forest","jungle","swamp","taiga","tundra","glacier","hills","mount","peaks"};
  printf("  biomes:"); for(int i=0;i<BIOME_COUNT;i++) printf(" %s=%.1f%%",bn[i],100.0*cnt[i]/(W*W)); printf("\n");
  printf("  sites: village %d castle %d cave %d tower %d dungeon %d ruins %d fort %d temple %d\n",sites[1],sites[2],sites[3],sites[4],sites[5],sites[6],sites[7],sites[8]);
  int S=6; FILE*f=fopen(out,"wb"); fprintf(f,"P6\n%d %d\n255\n",W*S,W*S);
  for(int y=0;y<W*S;y++)for(int x=0;x<W*S;x++){ WorldTile*t=&m[y/S][x/S]; unsigned char c[3]; Col(t->biome,c);
    int lx=x%S,ly=y%S; bool center=(lx>=2&&lx<=3&&ly>=2&&ly<=3);
    float shade=0.75f+0.5f*(t->elevation>0.35f? (t->elevation-0.35f):0.f); for(int k=0;k<3;k++){ int v=(int)(c[k]*(t->biome>BIOME_LAKE?shade:1.f)); c[k]=v>255?255:v; }
    if(t->riverMask&&!t->isLake&&t->biome>BIOME_LAKE){c[0]=60;c[1]=130;c[2]=255;}
    else if(t->roadMask&&center){c[0]=120;c[1]=70;c[2]=30;}
    if(t->locationType&&center){ c[0]=255;c[1]=255;c[2]=255; if(t->locationType==1){c[0]=255;c[1]=60;c[2]=60;} }
    if(t->locationType&&(lx<=1||lx>=4||ly<=1||ly>=4)&&t->locationType==1){} 
    fwrite(c,1,3,f);} fclose(f);
}
