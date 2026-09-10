local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.action(14,0);local inventory=emu:saveStateBuffer()
 emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20)
 H.log('FIXTURE temporary TABIDACHI enemy-spawn gate in test RAM only')
 H.warp(0,0,504,504);H.option('freezeEnemies',1)
 local enemy
 for i=0,71 do local e=0x30015A0+i*0x88;if H.r32(e+4)~=0 and H.r8(e+8)==3 and H.r8(e+9)==0 then enemy=e;break end end
 assert(enemy,'native Octorok missing');H.wait(60)
 emu:write32(H.P+44,H.r32(enemy+44));emu:write32(H.P+48,H.r32(enemy+48))
 for i=1,80 do H.wait(1);if H.r8(H.P+66)>0 then break end end
 H.check(H.r8(H.P+66)>0,'real enemy contact initiates native Link knockback')
 local hit=emu:saveStateBuffer();local samples={}
 for mode=0,2,2 do
  emu:loadStateBuffer(hit);emu:setKeys(0);H.option('speedMode',mode);samples[mode]={}
  for t=1,35 do H.wait(1);samples[mode][t]={H.r32(H.P+44),H.r32(H.P+48),H.r32(H.P+52),H.r8(H.P+66)} end
 end
 local active,equal=0,true
 for t=1,35 do local a,b=samples[0][t],samples[2][t];if a[4]>0 then active=active+1;for j=1,4 do if a[j]~=b[j] then equal=false end end end end
 H.check(active>3 and equal,'Walk Speed x2 leaves actual native knockback unchanged ('..active..' active frames)')
 emu:loadStateBuffer(inventory);H.warp(0x0B,0,400,400)
 local ap=H.r32(0x800027C);local tx,ty
 for y=4,math.floor(H.r16(H.R+32)/16)-5 do for x=4,math.floor(H.r16(H.R+30)/16)-9 do
  local water=true
  for dx=-1,3 do for dy=-1,1 do if H.r8(ap+(y+dy)*64+x+dx)~=16 then water=false end end end
  if water and not tx then tx,ty=x,y end
 end end
 assert(tx,'wide native water fixture missing')
 emu:write32(H.P+44,(H.r16(H.R+6)+tx*16+8)*65536);emu:write32(H.P+48,(H.r16(H.R+8)+ty*16+8)*65536)
 H.log('FIXTURE position on native water, Flippers owned via Give All; no swim-state injection')
 H.wait(45);H.check(H.r8(H.S+38)~=0,'native water logic starts actual swimming')
 local swimming=emu:saveStateBuffer();samples={}
 for mode=0,2 do
  emu:loadStateBuffer(swimming);emu:setKeys(0);H.option('speedMode',mode==0 and 0 or 2);H.option('noClip',mode==2 and 1 or 0);samples[mode]={}
  for t=1,30 do H.wait(1,0x10);samples[mode][t]={H.r32(H.P+44),H.r32(H.P+48),H.r32(H.P+52),H.r8(H.S+38)} end
  H.shot('native_swim_'..mode)
 end
 for mode=1,2 do
  equal=true;active=0
  for t=1,30 do local a,b=samples[0][t],samples[mode][t];if a[4]~=0 then active=active+1;for j=1,4 do if a[j]~=b[j] then equal=false end end end end
  H.check(active>20 and equal,'native swim trajectory unchanged: Speed x2, No Clip '..(mode==2 and 'ON' or 'OFF')..' ('..active..' swim frames)')
 end
end)
