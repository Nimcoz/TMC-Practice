-- Real Cave of Flames floor, not synthetic tiles or injected player actions.
-- TMC_HARDWARE_HARNESS selects the existing regional gameplay harness.
local H=assert(loadfile(assert(os.getenv('TMC_HARDWARE_HARNESS'))))()
H.run(function()
 H.option('noClip',1)
 H.warp(0x50,8,120,120)
 local region=H.r8(0x080000AF)
 local actPtr=region==0x50 and 0x080002C4 or 0x0800027C
 -- JP's low-level tile pointer constants are relocated too; supplied by the
 -- regional harness through this read-only fixture environment value.
 if os.getenv('TMC_LAVA_ACT_POINTER') then actPtr=tonumber(os.getenv('TMC_LAVA_ACT_POINTER')) end
 local ap=H.r32(actPtr)
 local ox,oy,w,h=H.r16(H.R+6),H.r16(H.R+8),H.r16(H.R+30),H.r16(H.R+32)
 H.log(string.format('LAVA map region=%02X origin=%d,%d size=%d,%d act=%08X',region,ox,oy,w,h,ap))
 local tx,ty
 for y=2,math.floor(h/16)-3 do
  for x=2,math.floor(w/16)-8 do
   local clear=true
   for dx=0,6 do
    if H.surface(H.r8(ap+y*64+x+dx))~=0x1C then clear=false end
   end
   if clear then tx,ty=x,y;break end
  end
  if tx then break end
 end
 assert(tx,'no seven-tile native lava runway in fixture')
 H.warp(0x50,8,tx*16+8,ty*16+8)
 H.wait(8)
 H.check(H.r8(H.S+18)==0x1C,'standing on native lava surface 1C')
 local base=emu:saveStateBuffer()
 for _,on in ipairs({0,1}) do
  emu:loadStateBuffer(base);emu:setKeys(0);H.option('noClip',on)
  local x=H.r32(H.P+44);local health=H.r8(0x02002AEA)
  local lava,burn,fall=false,false,false
  for t=1,55 do
   H.wait(1,0x10)
   lava=lava or H.r8(H.P+12)==17 or H.r8(H.S+12)==17
   burn=burn or (H.r32(H.S+48)&0x400)~=0
   fall=fall or H.r32(H.P+52)~=0
  end
  local moved=(H.r32(H.P+44)-x)/65536
  H.log(string.format('LAVA on=%d dx=%.2f health=%d->%d lavaAction=%s burning=%s zChanged=%s',on,moved,health,H.r8(0x02002AEA),tostring(lava),tostring(burn),tostring(fall)))
  H.shot('lava_'..on)
  if on==0 then
   H.check(lava and burn,'No Clip OFF preserves native lava action and burning')
  else
   H.check(moved>65 and not lava and not burn and not fall,'No Clip ON walks across lava without burn / bounce / respawn')
   H.check(H.r8(0x02002AEA)==health,'lava walking does not lose health')
   H.check(H.r8(H.R+4)==0x50 and H.r8(H.R+5)==8,'lava walking does not force a room warp')
   -- Switch OFF while still on lava: restore the native hazard immediately.
   H.option('noClip',0);local restored=false
   for t=1,24 do H.wait(1);restored=restored or H.r8(H.P+12)==17 or H.r8(H.S+12)==17 end
   H.check(restored,'disabling No Clip over lava restores native hazard')
  end
 end
end)
