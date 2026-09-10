local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function floor()
 local cp,ap=H.r32(0x800024C),H.r32(0x800027C)
 local ox,oy,w,h=H.r16(H.R+6),H.r16(H.R+8),H.r16(H.R+30),H.r16(H.R+32)
 for ty=2,h//16-2 do for tx=2,w//16-4 do
  local ok=true
  for dx=0,3 do for dy=-1,0 do local p=(ty+dy)*64+tx+dx
   if H.r8(cp+p)~=0 or H.surface(H.r8(ap+p))~=0 then ok=false end
  end end
  local x,y=ox+tx*16+8,oy+ty*16+8
  for i=0,71 do local p=0x30015A0+i*136
   if H.r32(p+4)~=0 and math.abs(H.r16(p+46)-(x+32))<16 and math.abs(H.r16(p+50)-y)<16 then ok=false end
  end
  if ok then emu:write32(H.P+44,x*65536);emu:write32(H.P+48,y*65536);emu:write8(H.P+20,2);H.wait(8);return end
 end end
 error('no suitable ordinary ground in interior')
end
H.run(function()
 emu:write8(H.PM+16,1)
 for _,room in ipairs({{0x22,0x10,88,88,'house'},{0x48,0,120,120,'temple'},{0x80,0,216,256,'castle'}}) do
  H.warp(room[1],room[2],room[3],room[4]);H.wait(30)
  for kind=0,4 do
   floor();H.menu(29,1);H.option('actorSpawn',kind);H.press(1);H.wait(85);H.close()
   local e=H.r32(H.PM+H.L.actorSelected)
   H.check(e>=0x30015A0 and e<0x3003BE0,room[5]..' native spawn '..kind)
   assert(e>=0x30015A0 and e<0x3003BE0,'spawn rejected: '..emu:readRange(H.PM+H.L.status,28))
   local m=H.PM+H.L.actorMarks+(e-0x30015A0)//136
   H.menu(28,0);H.shot(room[5]..'_detail_'..kind)
   if kind<3 then
    H.press(1);H.close();H.wait(10)
    local x,y,t=H.r32(e+44),H.r32(e+48),H.r8(e+14);H.wait(60)
    H.check((H.r8(m)&1)==1 and H.r32(e+44)==x and H.r32(e+48)==y and H.r8(e+14)==t,room[5]..' individual freeze '..kind)
    H.shot(room[5]..'_frozen_'..kind)
   end
   H.menu(28,1);H.press(1);H.check(H.r8(H.PM+H.L.page)==30,room[5]..' removable curated actor '..kind)
   H.press(0x80);H.press(0x301);H.wait(50);H.close()
   H.check(H.r32(H.PM+H.L.actorSelected)==0 and H.r8(m)==0,room[5]..' full native cleanup '..kind)
   H.check(H.r8(H.R+4)==room[1] and H.r8(H.R+5)==room[2],room[5]..' no artificial warp '..kind)
  end
 end
end)
