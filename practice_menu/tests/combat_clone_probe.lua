local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local L=assert(loadfile(out..'layout.lua'))()
local f=assert(io.open(out..'trace.log','w'))
local PM,P,S,M,R=0x203D000,0x3001160,0x3003F80,0x3001000,0x3000BF0
local n,ready,start,e,tiles=0,0,nil,nil,{}
local function log(s) f:write(string.format('%04d %s\n',n,s)) end
local function state()
 log(string.format('room=%02X/%02X main=%d/%d/%d action=%d xy=%d,%d charge=%d flags=%08X skills=%04X clones=%08X,%08X,%08X',emu:read8(R+4),emu:read8(R+5),emu:read8(M+2),emu:read8(M+3),emu:read8(M+4),emu:read8(P+12),emu:read16(P+46),emu:read16(P+50),emu:read8(S+160),emu:read32(S+48),emu:read16(S+172),emu:read32(0x3004040),emu:read32(0x3004044),emu:read32(0x3004048)))
 if e then log(string.format('enemy=%08X id=%02X xy=%d,%d hp=%d action=%d/%d contact=%02X knock=%d confused=%d next=%08X',e,emu:read8(e+9),emu:read16(e+46),emu:read16(e+50),emu:read8(e+69),emu:read8(e+12),emu:read8(e+13),emu:read8(e+65),emu:read8(e+66),emu:read8(e+67),emu:read32(e+4))) end
end
local function warp(a,r,x,y)
 emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
 emu:write8(0x30010AC,a);emu:write8(0x30010AD,r);emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
 emu:write16(0x30010B0,x);emu:write16(0x30010B2,y);emu:write8(0x30010B4,1)
end
local function finish(s)
 state();log(s);f:close();local d=assert(io.open(out..'done.txt','w'));d:write(s);d:close();start=-1
end
local function scan()
 tiles={}
 local ptr=emu:read32(0x8000278+4)
 local ox,oy=emu:read16(R+6),emu:read16(R+8)
 local w,h=emu:read16(R+30),emu:read16(R+32)
 for y=0,math.floor(h/16)-1 do for x=0,math.floor(w/16)-1 do
  if emu:read8(ptr+y*64+x)==87 then
   local p={x=ox+x*16+8,y=oy+y*16+8};tiles[#tiles+1]=p;log(string.format('CLONE_TILE %d,%d',p.x,p.y))
  end
 end end
 log('CLONE_TILES_COUNT='..#tiles)
 if #tiles>0 then emu:write32(P+44,tiles[1].x*65536);emu:write32(P+48,tiles[1].y*65536) end
end
callbacks:add('frame',function()
 n=n+1;local k=0
 if not start then
  if emu:read8(M+2)==0 and n>650 and n%120<4 then k=8 end
  if emu:read8(M+2)==1 and n%120<4 then k=1 end
  if emu:read8(M+2)==2 and emu:read8(M+3)==2 and emu:read8(M+4)==2 then
   ready=ready+1;if ready>45 then start=n;log('NATIVE_BOOT') end
  end
  if n>2500 then finish('FAIL_BOOT') end
 elseif start>=0 then
  local t=n-start
  if t<4 then k=0x304 end
  if t==35 then emu:write8(PM+L.page,14);emu:write8(PM+L.cursor+14,0) end
  if t>=40 and t<43 then k=1 end
  if t>=60 and t<64 then k=0x304 end
  if t==110 then
   -- TEST FIXTURE ONLY: native LoadRoom gates its enemy list on TABIDACHI.
   -- It is absent in the supplied pre-adventure save. No file is saved.
   emu:write8(0x02002C9E,emu:read8(0x02002C9E)|0x20)
   log('FIXTURE_TABIDACHI=1; NOT A RELEASE PATCH OR SAVE WRITE')
   emu:write8(PM+16,1);warp(0,0,0x1F8,0x1F8)
  end
  if t==250 then
   for i=0,71 do local p=0x30015A0+i*0x88
    if emu:read8(p+8)==3 and emu:read32(p+4)~=0 then
     log(string.format('LOADED_ENEMY %08X id=%02X xy=%d,%d hp=%d',p,emu:read8(p+9),emu:read16(p+46),emu:read16(p+50),emu:read8(p+69)))
     if not e and emu:read8(p+9)==0 then e=p end
    end
   end
   emu:write8(PM+L.freezeEnemies,1)
  end
  if t==290 and e then
   emu:write32(P+44,emu:read32(e+44));emu:write32(P+48,emu:read32(e+48)+20*65536)
   emu:write8(P+20,0);emu:write8(P+21,0);emu:screenshot(out..'enemy_before.png')
  end
  if t>=320 and t<470 and t%30<3 then k=1 end
  if t==475 then emu:screenshot(out..'enemy_after.png') end
  if t==500 then
   e=nil;emu:write8(PM+L.freezeEnemies,0)
   -- TEST FIXTURE ONLY: local bank 3 + 79 is Sanctuary intro completed.
   -- Must be set before LoadRoom decides whether to create its script actor.
   emu:write8(0x02002D0B,emu:read8(0x02002D0B)|3)
   log('FIXTURE_SANCTUARY_378_LEAVE_ALLOWED_379_INTRO=1; NO SAVE WRITE')
   emu:write8(0x02002B42,0) -- independent element ownership: none
   warp(0x78,1,312,600)
  end
  if t==650 then scan();emu:screenshot(out..'clone_tiles.png') end
  if t>=690 and t<1060 then k=1 end
  if t>=980 and t<1000 then k=k+0x10 end
  if t==990 or t==1040 then emu:screenshot(out..'clone_'..t..'.png') end
  if t==1120 then warp(0x78,0,120,120) end
  if t==1270 then scan();emu:screenshot(out..'clone_hall.png') end
  if t>=1300 and t<1690 then k=1 end
  if t>=1590 and t<1610 then k=k+0x10 end
  if t==1640 then emu:screenshot(out..'clone_hall_active.png') end
  if t%10==0 then state() end
  if t==1750 then finish('PROBE_COMPLETE_REQUIRES_REVIEW') end
 end
 emu:setKeys(k)
end)
