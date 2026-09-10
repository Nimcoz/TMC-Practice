-- Reproduce the real staircase glitch with native item and transition code.
-- No write to PL_USE_OCARINA / priority / action is used to manufacture OG.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local L=assert(loadfile(out..'layout.lua'))()
local f=assert(io.open(out..'trace.log','w'))
local PM,P,S,M,R=0x203D000,0x3001160,0x3003F80,0x3001000,0x3000BF0
local n,ready,start,base,candidates,trial,found=0,0,nil,nil,{},0,nil
local function log(s) f:write(n..' '..s..'\n') end
local function status(tag)
 log(string.format('%s trial=%d room=%02X/%02X xy=%d,%d action=%d/%d flags=%08X prio=%d pprio=%02X item=%08X slot=%02X/%02X floor=%d mobility=%02X keep=%02X field27=%02X unk7a=%04X',tag,trial,emu:read8(R+4),emu:read8(R+5),emu:read16(P+46),emu:read16(P+50),emu:read8(P+12),emu:read8(P+13),emu:read32(S+48),emu:read8(0x3003DC0),emu:read8(P+11),emu:read32(S+44),emu:read8(0x3000B80),emu:read8(0x3000B81),emu:read8(S+18),emu:read8(S+26),emu:read8(S+11),emu:read8(S+39),emu:read16(P+122)))
end
local function finish(s)
 status(s);f:close();local d=assert(io.open(out..'done.txt','w'));d:write(s);d:close();start=-1
end
local function warp()
 emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
 emu:write8(0x30010AC,0x22);emu:write8(0x30010AD,0x10);emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
 emu:write16(0x30010B0,88);emu:write16(0x30010B2,60);emu:write8(0x30010B4,1)
end
local function scan()
 local ptr=emu:read32(0x8000278+4)
 local ox,oy=emu:read16(R+6),emu:read16(R+8)
 local w,h=emu:read16(R+30),emu:read16(R+32)
 for y=0,math.floor(h/16)-1 do for x=0,math.floor(w/16)-1 do
  local a=emu:read8(ptr+y*64+x)
  if a==63 or a==241 or a==40 or a==41 then
   log(string.format('STAIR x=%d y=%d act=%d',ox+x*16,oy+y*16,a))
   for dy=8,27 do candidates[#candidates+1]={x=ox+x*16+8,y=oy+y*16+dy} end
  end
 end end
 base=emu:saveStateBuffer();log('CANDIDATES='..#candidates)
end
callbacks:add('frame',function()
 n=n+1;local k=0
 if not start then
  if emu:read8(M+2)==0 and n>650 and n%120<4 then k=8 end
  if emu:read8(M+2)==1 and n%120<4 then k=1 end
  if emu:read8(M+2)==2 and emu:read8(M+3)==2 and emu:read8(M+4)==2 then
   ready=ready+1;if ready>45 then start=n end
  end
  if n>2500 then finish('FAIL_BOOT') end
 elseif start>=0 then
  local t=n-start
  if t<4 then k=0x304 end
  if t==35 then emu:write8(PM+L.page,14);emu:write8(PM+L.cursor+14,0) end
  if t>=40 and t<43 then k=1 end
  if t>=60 and t<64 then k=0x304 end
  if t==110 then emu:write8(0x02002AF4,23);warp() end
  if t==250 then emu:screenshot(out..'stairs_room.png');scan() end
  if found then
   local q=n-found
   if q>=10 and q<26 then k=0x80 end
   if q==30 then status('MOVE_AFTER_GLITCH');emu:screenshot(out..'native_glitch.png');emu:saveStateFile(out..'native_glitch.ss0') end
   if q>=50 and q<54 then k=0x304 end
   if q==85 then status('MENU_IN_GLITCH');emu:screenshot(out..'glitch_menu.png') end
   if q>=100 and q<104 then k=0x304 end
   if q==150 then status('AFTER_MENU');finish('NATIVE_GLITCH_REPRODUCED_REVIEW_REQUIRED') end
  elseif t>=260 then
   local q=(t-260)%50
   if q==0 then
    trial=trial+1
    if trial>#candidates then finish('NO_GLITCH_IN_STAIR_CANDIDATES');return end
    assert(emu:loadStateBuffer(base))
    emu:write32(P+44,candidates[trial].x*65536);emu:write32(P+48,candidates[trial].y*65536)
    emu:write8(P+20,0);emu:write8(P+21,0)
    status('SETUP')
   end
   if q>=6 and q<=9 then k=0x40 end
   if q==7 then k=k+1 end
   if q==11 or q==20 or q==40 then status('CHECK') end
   if q==40 and (emu:read32(S+48)&0x10000000)~=0 and emu:read8(P+12)==1 and emu:read8(0x3000B81)~=23 then
    found=n;status('CANDIDATE_NATIVE_OG')
    local d=assert(io.open(out..'native_pstate.bin','wb'));d:write(emu:readRange(S,0xB0));d:close()
   end
  end
 end
 emu:setKeys(k)
end)
