local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local L=assert(loadfile(out..'layout.lua'))()
local n,done=0,false
callbacks:add('frame',function()
 if done then return end;n=n+1
 local p=L.autoState
 if emu:read32(p)~=0x4F545541 then
  if n>3000 then local f=assert(io.open(out..'done.txt','w'));f:write('FAIL AUTO STATE ADDRESS');f:close();done=true end
  return
 end
 if emu:read8(p+52)==13 or n>4000 then
  local f=assert(io.open(out..'trace.log','w'))
  local status,layout,passed,failed,movement=emu:read32(p+4),emu:read32(p+36),emu:read32(p+40),emu:read32(p+44),emu:read32(p+48)
  local result=string.format('status=%08X layout=%08X passed=%08X failed=%08X movement=%08X phase=%d',status,layout,passed,failed,movement,emu:read8(p+52))
  f:write(result);f:close();f=assert(io.open(out..'done.txt','w'))
  f:write((status==0x7F and layout==0x7FF and passed==0x3FFF and failed==0 and movement==0x10F and 'PASS ' or 'FAIL ')..result);f:close();done=true
 end
end)
