-- Native boot and real framebuffer sequences. RAM writes only select overlay
-- options or establish a native room-transition fixture, never render pixels.
local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local f=assert(io.open(out..'trace.log','w'))
local n,ready,start=0,0,nil
local M,PM,P=0x03001000,0x0203D000,0x03001160
local function shot(name) emu:screenshot(out..name..'.png') end
local function log(tag)
 f:write(string.format('%s n=%d main=%d/%d/%d area=%02X room=%02X xy=%d,%d bg0=%04X/%d/%d font=%08X\n',tag,n,emu:read8(M+2),emu:read8(M+3),emu:read8(M+4),emu:read8(0x3000BF4),emu:read8(0x3000BF5),emu:read16(P+46),emu:read16(P+50),emu:read16(0x3000F58),emu:read16(0x3000F5A),emu:read16(0x3000F5C),emu:read32(0x600DEC0)))
end
local function finish(s)
 log(s); f:close(); local d=assert(io.open(out..'done.txt','w'));d:write(s);d:close();start=-1
end
callbacks:add('frame',function()
 n=n+1;local k=0
 if not start then
  if emu:read8(M+2)==0 and n>650 and n%120<4 then k=8 end
  if emu:read8(M+2)==1 and n%120<4 then k=1 end
  if emu:read8(M+2)==2 and emu:read8(M+3)==2 and emu:read8(M+4)==2 then
   ready=ready+1;if ready>45 then start=n;log('NATIVE_BOOT');shot('00_clean') end
  end
  if n>2500 then finish('FAIL_BOOT') end
 elseif start>=0 then
  local t=n-start
  if t==10 then emu:write8(PM+12,1) end
  if t>=20 and t<25 then shot('timer_'..t) end
  if t==40 then emu:write8(PM+12,0);emu:write32(PM+8,0);emu:write8(PM+22,1) end
  if t>=50 and t<55 then shot('debug_'..t) end
  if t==70 then emu:write8(PM+12,1) end
  if t>=80 and t<85 then shot('both_'..t) end
  if t==100 then emu:write8(PM+12,0);emu:write32(PM+8,0);emu:write8(PM+22,0) end
  if t==110 then shot('01_cleared') end
  if t==120 then emu:write8(PM+12,1);emu:write8(PM+22,1) end
  if t>=130 and t<134 then k=0x304 end
  if t==160 then shot('02_menu') end
  if t>=170 and t<174 then k=0x304 end
  if t>=210 and t<215 then shot('reopened_'..t) end
  -- South Hyrule Field, actual AREA_HYRULE_FIELD / SOUTH_HYRULE_FIELD.
  if t==240 then
   emu:write8(0x30010A8,1);emu:write8(0x30010A9,5)
   emu:write8(0x30010AC,3);emu:write8(0x30010AD,1)
   emu:write8(0x30010AE,4);emu:write8(0x30010AF,0)
   emu:write16(0x30010B0,0x150);emu:write16(0x30010B2,0x110);emu:write8(0x30010B4,1)
  end
  if t>=340 and t<440 then k=0x10 end
  if t>=390 and t<400 then shot('scroll_'..t) end
  if t==460 then emu:write8(PM+12,0);emu:write32(PM+8,0);emu:write8(PM+22,0) end
  if t>=470 and t<475 then shot('scroll_clear_'..t) end
  if t%20==0 then log('FRAME') end
  if t==500 then finish('FRAMEBUFFERS_CAPTURED_REQUIRES_REVIEW') end
 end
 emu:setKeys(k)
end)
