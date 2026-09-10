-- Real native boot, menu actions and controller input. Fixture RAM changes are
-- explicitly logged. No PASS is inferred from a screenshot being created.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local L=assert(loadfile(out..'layout.lua'))()
local f=assert(io.open(out..'trace.log','w'))
local PM,P,S,M,R=0x203D000,0x3001160,0x3003F80,0x3001000,0x3000BF0
local n,ready,co,finished,failed=0,0,nil,false,0
local function log(s) f:write(string.format('%05d %s\n',n,s));f:flush() end
local function r8(a) return emu:read8(a) end
local function r16(a) return emu:read16(a) end
local function r32(a) return emu:read32(a) end
local function option(name,v) emu:write8(PM+assert(L[name],name),v) end
local function wait(count,k) for i=1,count do coroutine.yield(k or 0) end end
local function press(k) wait(2,k);wait(4) end
local function check(ok,s) log((ok and 'PASS ' or 'FAIL ')..s);if not ok then failed=failed+1 end end
local function shot(s) emu:screenshot(out..s..'.png') end
local function state(s) log(string.format('%s area=%02X/%02X xy=%d,%d z=%08X action=%d ctrl=%d flags=%08X prio=%d/%02X pause=%d anim=%04X charge=%d',s,r8(R+4),r8(R+5),r16(P+46),r16(P+50),r32(P+52),r8(P+12),r8(S+139),r32(S+48),r8(0x3003DC0),r8(P+17),r8(0x2034490),r16(S+8),r8(S+160))) end
local function menu(page,row)
 if r8(PM+13)==0 then press(0x304);wait(30) end
 assert(r8(PM+13)==1,'menu failed to open')
 option('page',page);emu:write8(PM+L.cursor+page,row or 0);wait(2)
end
local function close()
 if r8(PM+13)==1 then press(0x304) end
 for i=1,150 do
  if r8(M+3)==2 and r8(M+4)==2 and r8(0x3000FD0)==0 then wait(4);return end
  wait(1)
 end
 error('menu failed to return to gameplay')
end
local function action(page,row) menu(page,row);press(1);close() end
local function warp(a,r,x,y)
 log(string.format('FIXTURE native transition to %02X/%02X at %d,%d',a,r,x,y))
 emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
 emu:write8(0x30010AC,a);emu:write8(0x30010AD,r);emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
 emu:write16(0x30010B0,x);emu:write16(0x30010B2,y);emu:write8(0x30010B4,1)
 wait(170);check(r8(R+4)==a and r8(R+5)==r,'native fixture room loaded');state('WARP_SETTLED')
end
local function flags() return emu:readRange(0x2002C9C,0x200) end
local function item(i) return (r8(0x2002B32+math.floor(i/4)) >> ((i%4)*2))&3 end
local function restore(buf) emu:loadStateBuffer(buf);wait(2) end
local function run()
 log('NATIVE_BOOT with untouched supplied save');state('BOOT')
 local initialFlags=flags()
 local function elementFlags(mask)
  local v=initialFlags:byte(1)&(~0x6C)
  for i,bit in ipairs({2,3,5,6}) do if mask&(1<<(i-1))~=0 then v=v|(1<<bit) end end
  return string.char(v)..initialFlags:sub(2)
 end
 action(14,0)
 check(item(6)==1 and r8(0x2002AF4)==6,'Give All grants and equips Four Sword')
 check(flags()==elementFlags(15),'Give All sets only four element-clear flags; other story flags unchanged')
 wait(25,1);state('FOUR_SWORD_CHARGE');check(r16(S+8)~=0,'Four Sword animation initialized before Ezlo')
 wait(60);local x=r32(P+44);wait(16,0x20);wait(3);check(r32(P+44)~=x,'movement after Four Sword charge cancellation')
 for i=1,3 do press(1);wait(32) end
 state('REPEATED_SWINGS');check(r8(P+12)==1 and r16(S+8)~=0,'repeated Four Sword swings complete')
 -- All independent element combinations, actual menu edits. No boss resets.
 menu(15,0)
 for mask=0,15 do
  for i=0,3 do if item(64+i)~=((mask>>i)&1) then emu:write8(PM+L.cursor+15,i);press(1) end end
  local actual=0;for i=0,3 do actual=actual|(item(64+i)<<i) end
  check(actual==mask,'independent elements mask '..mask)
  check(flags()==elementFlags(mask),'matching element-clear flags only, mask '..mask)
 end
 close();check(flags()==elementFlags(15),'element changes leave all other story flags unchanged')
 -- Delete All default NO, then explicit YES. Real equipped and charged items.
 menu(3,5);press(1);check(r8(PM+L.page)==17 and r8(PM+L.cursor+17)==0,'Delete All opens with NO selected')
 press(1);close();check(item(6)==1,'Delete All NO preserves inventory')
 action(17,1);local allZero=true;for i=1,135 do if item(i)~=0 then allZero=false end end
 check(allZero and r16(0x2002AF4)==0 and r16(S+172)==0,'Delete All clears ownership equipment and learned skills')
 check(r32(0x3004040)==0 and r32(0x3004044)==0 and r32(0x3004048)==0,'Delete All clears clone pointers')
 check(flags()==elementFlags(0),'Delete All clears element ownership/clear flags only; other story flags unchanged')
 -- Manual sword selection from NONE, all valid versions; five is deliberately skipped.
 for _,id in ipairs({1,2,3,4,6}) do
  menu(4,0);press(0x10);close();check(item(id)==1 and item(5)==0,'manual sword version '..id)
  emu:write8(0x2002AF4,id);log('FIXTURE equipment A='..id..' (native inventory owns weapon)')
  wait(24,1);wait(50);local before=r32(P+44);wait(6,0x20);wait(2)
  check(r16(S+8)~=0 and r32(P+44)~=before,'manual sword '..id..' charge/cancel/move')
 end
 action(14,0)
 -- Native house floor, also the location of the independently reproduced glitch.
 warp(0x22,0x10,88,88)
 local baseline=emu:saveStateBuffer()
 action(16,3);wait(20);state('MENU_OG_START');shot('og_start')
 check((r32(S+48)&0x10000000)~=0 and r8(0x3003DC0)==6 and r8(0x2034490)==1,'Ocarina Glitch enters observed native stage 1')
 local oy=r32(P+48);wait(10,0x80);wait(2);check(r32(P+48)~=oy,'OG stage 1 keeps walking active')
 menu(16,3);shot('og_menu');close();state('OG_AFTER_MENU')
 check((r32(S+48)&0x10000000)~=0 and r8(0x3003DC0)==6,'menu preserves OG priority rather than forcing stage 2')
 action(16,3);wait(8);state('MENU_OG_END')
 check((r32(S+48)&0x10000000)==0 and r8(0x3003DC0)==0 and r8(0x2034490)==0,'menu ends OG native flags and priority')
 restore(baseline);action(16,3);emu:write8(0x2002AF4,23);press(1);wait(190);state('NATIVE_OCARINA_CANCEL')
 check((r32(S+48)&0x10000000)==0 and r8(0x3003DC0)==0 and r8(0x2034490)==0,'playing native Ocarina ends menu-created glitch')
 restore(baseline)
 -- Menu last-page/cursor memory.
 menu(2,2);close();press(0x304);wait(30)
 check(r8(PM+L.page)==2 and r8(PM+L.cursor+2)==2,'Player page and cursor survive close/reopen');close()
 -- Compare walking and native roll on a roomy outdoor floor from identical states.
 warp(0,0,504,504)
 -- Find a long open native floor strip. No state/action/physics forcing.
 local cp,ap=r32(0x8000248+4),r32(0x8000278+4)
 local ox,oy,w,h=r16(R+6),r16(R+8),r16(R+30),r16(R+32)
 local found=false
 local function surface(act)
  for p=0x8007CAC,0x8007DC0,4 do local key=r16(p);if key==0 then return 0 end;if key==act then return r16(p+2) end end
  return 0
 end
 for ty=6,math.floor(h/16)-7 do
  for tx=5,math.floor(w/16)-13 do
   local clear=true
   for dx=0,10 do for dy=-1,0 do
    local j=(ty+dy)*64+tx+dx
    if r8(cp+j)~=0 or surface(r8(ap+j))~=0 then clear=false end
   end end
   if clear and not found then
    emu:write32(P+44,(ox+tx*16+8)*65536);emu:write32(P+48,(oy+ty*16+8)*65536)
    log('FIXTURE position at verified open native floor strip');found=true
   end
  end
 end
 assert(found,'no open floor fixture found');wait(35);state('OPEN_FLOOR')
 baseline=emu:saveStateBuffer();shot('movement_start')
 local distances={}
 for speed=0,2 do
  restore(baseline);option('speedMode',speed);local sx=r32(P+44);wait(24,0x10);wait(2)
  distances[speed]=math.abs(r32(P+44)-sx)/65536;state('SPEED_'..speed);log('distance='..distances[speed])
 end
 check(distances[0]>0 and distances[1]>distances[0]*1.3 and distances[2]>distances[0]*1.8,'speed affects controlled walking at 1 / 1.5 / 2x')
 restore(baseline);option('lockDirection',1);local face=r8(P+20);wait(16,0x10);wait(2)
 check(r8(P+20)==face and r32(P+44)~=0,'Lock Direction preserves facing during movement')
 option('lockDirection',0);wait(4,0x20);wait(2);check(r8(P+20)~=face,'Lock Direction OFF restores native facing')
 restore(baseline);action(9,3);local cx,cy=r16(R+10),r16(R+12);local sx=r32(P+44);wait(36,0x10);wait(2)
 check(r8(PM+L.cameraActive)==1 and r16(R+10)==cx and r16(R+12)==cy and r32(P+44)~=sx,'Camera Lock freezes camera but permits player movement')
 check(r32(R+48)==P,'Camera Lock does not steal persistent native target')
 restore(baseline);action(9,4);sx=r32(P+44);local sy=r32(P+48);cx=r16(R+10);wait(24,0x110);wait(2);shot('free_camera')
 check(r8(PM+L.cameraActive)==1 and r32(P+44)==sx and r32(P+48)==sy and r16(R+10)~=cx,'Free Camera moves view without moving Link')
 press(2);wait(3);check(r8(PM+L.cameraMode)==0 and r32(R+48)==P,'Free Camera B releases native target')
 restore(baseline);sx=r32(P+44);action(18,1);check(r32(P+44)==sx+65536,'Position Nudge performs one pixel')
 option('nudgeStep',1);sx=r32(P+44);action(18,1);check(r32(P+44)==sx+8*65536,'Position Nudge performs eight pixels')
 -- Camera and speed must not stay attached across a real room transition.
 restore(baseline);action(9,3);warp(0x22,0x10,88,88)
 check(r8(PM+L.cameraActive)==0 and r32(R+48)==P,'room transition releases camera ownership')
 -- Known flag editing readback, then restore same bit; no implied boss reset.
 local before=flags();menu(19,1);press(1)
 check(r8(PM+L.flagAfter)==1 and (r8(0x2002CA2)&2)~=0,'known gold Octorok death flag set and read back')
 emu:write8(PM+L.cursor+19,2);press(1);check(r8(PM+L.flagBefore)==1 and r8(PM+L.flagAfter)==0,'known flag clear displays before/after')
 close();check(flags()==before,'known flag edit/clear leaves other flags unchanged')
 shot('completed');state('END');log('RESULT failures='..failed)
end
callbacks:add('frame',function()
 if finished then return end;n=n+1
 if not co then
  local k=0
  if r8(M+2)==0 and n>650 and n%120<4 then k=8 end
  if r8(M+2)==1 and n%120<4 then k=1 end
  if r8(M+2)==2 and r8(M+3)==2 and r8(M+4)==2 then ready=ready+1;if ready>45 then co=coroutine.create(run) end end
  emu:setKeys(k)
 else
  local ok,k=coroutine.resume(co)
  if not ok then failed=failed+1;log('FAIL HARNESS '..tostring(k)) end
  if not ok or coroutine.status(co)=='dead' then
   finished=true;f:close();local d=assert(io.open(out..'done.txt','w'));d:write('GAMEPLAY_CHECKS failures='..failed);d:close();emu:setKeys(0)
  else emu:setKeys(k or 0) end
 end
end)
