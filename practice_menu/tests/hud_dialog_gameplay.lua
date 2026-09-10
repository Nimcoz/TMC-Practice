local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.warp(0,0,504,504);H.openFloor()
 local baseFrame=H.r32(H.PM+4)
 local base=emu:saveStateBuffer()
 -- Identical input, native map frames, with each overlay combination.
 for mode=0,3 do
  emu:loadStateBuffer(base);emu:setKeys(0)
  while H.r32(H.PM+4)<baseFrame+10 do H.wait(1) end
  emu:write8(H.PM+12,mode&1);H.option('debugHud',(mode>>1)&1)
  while H.r32(H.PM+4)<baseFrame+20 do H.wait(1) end
  local before=H.r16(H.R+10)
  for t=1,92 do
   repeat H.wait(1,0x10) until H.r32(H.PM+4)>=baseFrame+20+t
   if t>=65 and t<=70 then H.shot('mode_'..mode..'_frame_'..t);H.log(string.format('SCANOUT mode=%d frame=%d x=%08X camera=%d gameframes=%d',mode,t,H.r32(H.P+44),H.r16(H.R+10),H.r32(H.PM+4))) end
  end
  H.check(H.r16(H.R+10)~=before,'actual camera scrolling, overlay mode '..mode)
  emu:write8(H.PM+12,0);emu:write32(H.PM+8,0);H.option('debugHud',0)
  while H.r32(H.PM+4)<baseFrame+117 do H.wait(1) end
  H.shot('mode_'..mode..'_cleared')
 end
 -- A native conversation with a real house NPC, not fabricated textbox bytes.
 H.warp(0x22,0x11,64,92)
 local house=emu:saveStateBuffer();local spoke=false
 local candidates={}
 for i=0,71 do local e=0x30015A0+i*0x88
  if H.r32(e+4)~=0 and H.r8(e+8)==7 then candidates[#candidates+1]={x=H.r32(e+44),y=H.r32(e+48),id=H.r8(e+9)} end
 end
 H.log('native NPC candidates '..#candidates)
 for _,npc in ipairs(candidates) do
  if not spoke then
   emu:loadStateBuffer(house);H.log('FIXTURE position below native NPC '..npc.id)
   emu:write32(H.P+44,npc.x);emu:write32(H.P+48,npc.y+20*65536);emu:write8(H.P+20,0);H.wait(6);H.press(0x100);H.wait(90)
   if (H.r8(0x2000050)&0x7F)~=0 then spoke=true end
  end
 end
 if not spoke then
  -- The supplied story state can withhold Smith's interaction. Use the actual
  -- Zelda scene and wait for its native textbox instead of fabricating one.
  H.warp(3,1,336,272)
  for i=1,250 do if (H.r8(0x2000050)&0x7F)~=0 then spoke=true;break end;H.wait(1) end
 end
 H.check(spoke,'real NPC conversation started')
 if spoke then
  emu:write8(H.PM+12,1);H.option('debugHud',1);H.wait(10)
  for t=1,5 do H.shot('dialog_before_'..t);H.wait(1) end
  local msg=H.r8(0x2000050);local id=H.r16(0x2000054)
  H.menu(12,0);H.shot('dialog_menu');H.close()
  H.check((H.r8(0x2000050)&0x7F)~=0 and H.r16(0x2000054)==id,'menu restores real conversation and message ID')
  for t=1,5 do H.shot('dialog_after_'..t);H.wait(1) end
  for i=1,20 do H.press(1);H.wait(12) end
  H.check((H.r8(0x2000050)&0x7F)==0,'conversation can complete after Practice Menu')
 end
 -- Native early-game scripted scene in South Hyrule Field.
 emu:loadStateBuffer(house);H.warp(3,1,336,272)
 H.check(H.r8(H.S+139)~=0,'native early-game scene is active')
 local ctrl=H.r8(H.S+139);local act=H.r8(H.P+12)
 H.menu(12,0);H.shot('cutscene_menu');H.close()
 H.check(H.r8(H.S+139)==ctrl and H.r8(H.P+12)==act,'opening / closing menu preserves native scene control')
 H.wait(160);H.shot('cutscene_resumed')
end)
