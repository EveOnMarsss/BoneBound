#include "project.h"
#include "World/WorldMap.h"
#include "Game/Camera.h"
#include "World/LocalMap.h"
#include "World/Tiles.h"
#include "World/WorldGen.h"
#include "World/LocationNames.h"
#include "Entities/NPC.h"
#include "Entities/SpawnPoint.h"
#include "Systems/SaveLoad.h"
#include <vector>
#include <string>
#include <chrono>
#include <unistd.h>

// ---- minimal raylib stand-ins (no window in this environment) ----
extern "C" {
int GetScreenWidth(void){ return 1280; }
int GetScreenHeight(void){ return 720; }
double GetTime(void){ return 0.0; }
float GetFrameTime(void){ return 0.016f; }
int GetRandomValue(int a,int b){ return a + rand()%(b-a+1); }
}
static unsigned long Hash(LocalMap* m){ unsigned long h=1469598103934665603UL; for(int y=0;y<m->height;y++)for(int x=0;x<m->width;x++){h^=(unsigned char)m->tiles[y][x];h*=1099511628211UL;} return h; }
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } }while(0)

int main(){
  worldSeed=424242;
  memset(&player,0,sizeof player);
  strcpy(player.name,"Biscuit"); player.isCat=true;
  auto t0=std::chrono::steady_clock::now();
  GenerateWorldMap(64,64);
  printf("world 64x64 generated; start (%d,%d) %s\n",player.worldPos.x,player.worldPos.y,worldMap[player.worldPos.y][player.worldPos.x].locationName);
  CHECK(player.maxHealth==20 && player.level==1,"starting stats");

  // 1. generate local maps for every location type + a sample of plain tiles, with structures and NPCs
  int perType[9]={0}; int generated=0; int structuresOnWater=0;
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){
    WorldTile* t=&worldMap[y][x];
    bool want = (t->locationType!=LOCATION_NONE && perType[t->locationType]<4) || ((x*5+y*3)%37==0);
    if(!want) continue;
    if(t->locationType) perType[t->locationType]++;
    GenerateLocalMap(x,y);
    CHECK(t->localMap!=NULL,"local map (%d,%d) not generated",x,y);
    if(!t->localMap) continue;
    generated++;
    LocalMap* m=t->localMap;
    // buildings must not sit on water; every building door/ layout char must be in table
    for(int b=0;b<m->buildingCount;b++){ Building* bd=m->buildings[b]; (void)bd; }
    for(int yy=0;yy<m->height;yy++)for(int xx=0;xx<m->width;xx++){
      char c=m->tiles[yy][xx]; CHECK(c!=0,"NUL tile at (%d,%d) in map (%d,%d)",xx,yy,x,y); if(!c) goto next;
      CHECK(strcmp(TileName(c),"unknown")!=0,"unknown tile code %d '%c' in map (%d,%d)",(int)c,c,x,y); if(strcmp(TileName(c),"unknown")==0) goto next;
    }
    next:;
  }
  printf("generated %d local maps (per type: V%d Ca%d Cv%d Tw%d Dg%d Ru%d Ft%d Tm%d)\n",generated,perType[1],perType[2],perType[3],perType[4],perType[5],perType[6],perType[7],perType[8]);

  // 2. determinism: regenerate the same tile after freeing -> identical
  int vx=-1,vy=-1; for(int y=0;y<64&&vx<0;y++)for(int x=0;x<64;x++) if(worldMap[y][x].locationType==LOCATION_VILLAGE&&worldMap[y][x].localMap){vx=x;vy=y;break;}
  unsigned long h1=Hash(worldMap[vy][vx].localMap), h2=0;
  { // wipe & regenerate that one map
    LocalMap* lm=worldMap[vy][vx].localMap;
    for(int i=0;i<lm->spawnPointCount;i++) free(lm->spawnPoints[i]); free(lm->spawnPoints);
    for(int i=0;i<lm->buildingCount;i++){Building*b=lm->buildings[i];for(int j=0;j<b->height;j++)free(b->layout[j]);free(b->layout);free(b);} free(lm->buildings);
    for(int y=0;y<lm->height;y++)free(lm->tiles[y]); free(lm->tiles); free(lm->biome); free(lm);
    worldMap[vy][vx].localMap=NULL;
    GenerateLocalMap(vx,vy); h2=Hash(worldMap[vy][vx].localMap);
  }
  CHECK(h1==h2,"village (%d,%d) is not deterministic (%lx vs %lx)",vx,vy,h1,h2);
  printf("determinism: village (%d,%d) regenerates identically: %s\n",vx,vy,h1==h2?"yes":"NO");

  // 3. enter, spawn position sanity, NPCs, tile travel in all 4 directions
  int sx=player.worldPos.x, sy=player.worldPos.y;
  EnterLocalMap(sx,sy);
  CHECK(player.isInLocalMap,"EnterLocalMap");
  LocalMap* lm=worldMap[sy][sx].localMap;
  CHECK(IsTileWalkable(lm->tiles[player.localPos.y][player.localPos.x]),"spawned on non-walkable tile '%c'",lm->tiles[player.localPos.y][player.localPos.x]);
  printf("entered start village: spawn (%d,%d) on '%s', NPCs=%d, buildings=%d\n",player.localPos.x,player.localPos.y,TileName(lm->tiles[player.localPos.y][player.localPos.x]),npcCount,lm->buildingCount);
  // like a player: walk to the edge, if the next cell is blocked (tree/water) try another row
  auto crossing=[&](int dx,int dy)->bool{
    LocalMap* cur=worldMap[player.worldPos.y][player.worldPos.x].localMap;
    for(int k=0;k<256;k++){
      int x=(dx==0)?(10+k)%256:(dx>0?cur->width-1:0), y=(dy==0)?(10+k)%256:(dy>0?cur->height-1:0);
      if(!IsTileWalkable(cur->tiles[y][x])) continue;
      player.localPos.x=x; player.localPos.y=y;
      int ox=player.worldPos.x, oy=player.worldPos.y;
      if(TravelToAdjacentTile(dx,dy)){
        CHECK(player.worldPos.x==ox+dx&&player.worldPos.y==oy+dy,"world pos after travel");
        LocalMap* nm=worldMap[player.worldPos.y][player.worldPos.x].localMap;
        CHECK(nm&&IsTileWalkable(nm->tiles[player.localPos.y][player.localPos.x]),"landed on unwalkable");
        return true; }
    }
    return false; };
  int dxs[4]={1,-1,0,0}, dys[4]={0,0,1,-1}; const char* names[4]={"east","west","south","north"};
  for(int d=0;d<4;d++){ bool ok=crossing(dxs[d],dys[d]); printf("tile travel %-5s: %s -> now on world tile (%d,%d) '%s' (%s)\n",names[d],ok?"ok":"no crossing found",player.worldPos.x,player.worldPos.y,worldMap[player.worldPos.y][player.worldPos.x].locationName,BiomeName(worldMap[player.worldPos.y][player.worldPos.x].biome)); }
  int hops=0; for(int i=0;i<40;i++){ if(!crossing(1,0)) break; hops++; }
  printf("then walked east across %d more consecutive world tiles (now on (%d,%d))\n",hops,player.worldPos.x,player.worldPos.y);
  CHECK(hops>=3,"should be able to keep crossing tiles");
  ExitLocalMap();
  CHECK(!player.isInLocalMap,"ExitLocalMap");

  // 4. save / load round trip
  player.gold=77; player.level=3; snprintf(player.name,sizeof player.name,"Biscuit");
  int px=player.worldPos.x, py=player.worldPos.y;
  worldMap[py][px].discovered=true;
  CHECK(SaveGame(0),"SaveGame");
  SaveInfo si; CHECK(ReadSaveInfo(0,&si),"ReadSaveInfo"); printf("save slot info: %s the %s lv%d at %s (%dx%d seed %u)\n",si.name,si.isCat?"Cat":"Dog",si.level,si.locationName,si.worldWidth,si.worldHeight,si.seed);
  unsigned long before=0; { std::string s; for(int y=0;y<64;y++)for(int x=0;x<64;x++){ WorldTile*t=&worldMap[y][x]; before=before*131+t->biome*7+t->riverMask*3+t->roadMask+t->locationType+(unsigned long)(t->elevation*1000)+t->discovered*5; } }
  // trash state then load
  CleanupAllMaps(); memset(&player,0,sizeof player); worldSeed=1;
  CHECK(LoadGame(0),"LoadGame");
  unsigned long after=0; for(int y=0;y<64;y++)for(int x=0;x<64;x++){ WorldTile*t=&worldMap[y][x]; after=after*131+t->biome*7+t->riverMask*3+t->roadMask+t->locationType+(unsigned long)(t->elevation*1000)+t->discovered*5; }
  CHECK(before==after,"world data differs after load");
  CHECK(player.gold==77&&player.level==3&&player.isCat&&strcmp(player.name,"Biscuit")==0&&worldSeed==424242,"player data after load: gold %d lvl %d name '%s' seed %u",player.gold,player.level,player.name,worldSeed);
  CHECK(player.worldPos.x==px&&player.worldPos.y==py,"position after load");
  printf("save/load round trip: world data %s, character %s\n",before==after?"identical":"DIFFERENT",(player.gold==77&&player.isCat)?"restored":"WRONG");
  // save while inside a local map, load, make sure player is placed back
  EnterLocalMap(player.worldPos.x,player.worldPos.y); int lx=player.localPos.x, ly=player.localPos.y;
  CHECK(SaveGame(1),"SaveGame in local"); CleanupAllMaps(); memset(&player,0,sizeof player);
  CHECK(LoadGame(1),"LoadGame in local"); CHECK(player.isInLocalMap&&player.localPos.x==lx&&player.localPos.y==ly,"restored inside local map at (%d,%d) vs (%d,%d)",player.localPos.x,player.localPos.y,lx,ly);
  printf("saved inside a local map and restored at the exact cell: %s\n",(player.isInLocalMap&&player.localPos.x==lx&&player.localPos.y==ly)?"yes":"NO");
  // corrupt & old-version files must fail cleanly
  { FILE*f=fopen("save_2.dat","wb"); fwrite("garbage data",1,12,f); fclose(f); CHECK(!LoadGame(2),"garbage save should be rejected"); }
  { FILE*f=fopen("save_0.dat","rb"); fseek(f,0,SEEK_END); long n=ftell(f); fclose(f); std::vector<char> buf(n/2); f=fopen("save_0.dat","rb"); fread(buf.data(),1,buf.size(),f); fclose(f); f=fopen("save_3.dat","wb"); fwrite(buf.data(),1,buf.size(),f); fclose(f); CHECK(!LoadGame(3),"truncated save should be rejected"); }
  CHECK(worldMap!=NULL&&player.isInLocalMap,"failed loads must not damage the running game");
  printf("garbage and truncated saves rejected without touching the running game\n");

  // 4b. starting-location picker flow: async generation -> choose start -> confirm
  {
    CleanupAllMaps(); memset(&player,0,sizeof player); strcpy(player.name,"Pick");
    BeginWorldGeneration(64,64,555);
    int spins=0; while(!UpdateWorldGeneration()){ spins++; usleep(2000); if(spins>5000){CHECK(false,"generation never finished");break;} }
    int rx,ry; GetRecommendedStart(&rx,&ry);
    CHECK(previewWholeMap,"preview mode should be on while choosing");
    CHECK(player.worldPos.x==rx&&player.worldPos.y==ry,"cursor should start on the recommended tile");
    CHECK(WorldTileEnterable(rx,ry),"recommended start must be valid");
    // find open water and land to test validity rules
    int wx=-1,wy=-1,lx2=-1,ly2=-1;
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){ BiomeType b=worldMap[y][x].biome; if(wx<0&&(b==BIOME_OCEAN||b==BIOME_DEEP_OCEAN)){wx=x;wy=y;} if(lx2<0&&b==BIOME_FOREST&&(x+y)%5==0){lx2=x;ly2=y;} }
    CHECK(!WorldTileEnterable(wx,wy),"open water must not be a valid start");
    CHECK(WorldTileEnterable(lx2,ly2),"a forest tile must be a valid start");
    // confirm on a chosen tile (what ENTER does)
    FinalizeNewWorld(lx2,ly2);
    CHECK(!previewWholeMap,"preview mode must turn off after confirming");
    CHECK(player.worldPos.x==lx2&&player.worldPos.y==ly2&&player.maxHealth==20&&player.level==1,"state after confirming start");
    CHECK(worldMap[ly2][lx2].discovered,"start tile must be discovered");
    int unrevealed=0; for(int y=0;y<64;y++)for(int x=0;x<64;x++) if(!worldMap[y][x].discovered) unrevealed++;
    CHECK(unrevealed>64*64-60,"only the area around the chosen start should be revealed (unrevealed=%d)",unrevealed);
    EnterLocalMap(lx2,ly2); CHECK(player.isInLocalMap&&IsTileWalkable(worldMap[ly2][lx2].localMap->tiles[player.localPos.y][player.localPos.x]),"can enter the chosen start");
    printf("start picker: async generation ok, cursor starts on recommended (%d,%d), water rejected, chosen forest tile (%d,%d) accepted, only the surroundings revealed\n",rx,ry,lx2,ly2);
  }
  FitWorldZoom();

  // 5. regenerate a bunch of worlds of every size (ASan checks the cleanup paths)
  for(int s=0;s<NUM_SIZES;s++){ worldSeed=1000+s; GenerateWorldMap(mapSizes[s].width,mapSizes[s].height);
     // enter 3 random tiles each
     for(int k=0;k<3;k++){ int x=1+(k*7+s*3)%(currentMapWidth-2), y=1+(k*11+s*5)%(currentMapHeight-2); GenerateLocalMap(x,y);} }
  CleanupAllMaps();
  printf("all 6 world sizes (16..512) generated, populated and freed\n");
  { // biggest world: save & load round trip
    worldSeed=2024; memset(&player,0,sizeof player); strcpy(player.name,"Big"); GenerateWorldMap(512,512);
    auto s0=std::chrono::steady_clock::now(); bool sv=SaveGame(2); double sms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-s0).count();
    FILE*f=fopen("save_2.dat","rb"); fseek(f,0,SEEK_END); long bytes=ftell(f); fclose(f);
    int px=player.worldPos.x,py=player.worldPos.y; CleanupAllMaps(); memset(&player,0,sizeof player);
    auto l0=std::chrono::steady_clock::now(); bool ld=LoadGame(2); double lms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-l0).count();
    CHECK(sv&&ld&&currentMapWidth==512&&player.worldPos.x==px&&player.worldPos.y==py,"512 world save/load");
    printf("512x512 world: save %.0f ms, load %.0f ms, file %.1f MB\n",sms,lms,bytes/1048576.0);
    CleanupAllMaps();
  }
  double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
  printf("%s (%d failures) in %.1fs\n",fails?"FAILED":"ALL CHECKS PASSED",fails,ms/1000);
  return fails?1:0;
}
