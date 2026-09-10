local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20) -- Existing TABIDACHI fixture, native town loader.
 emu:write8(0x2002C9C,H.r8(0x2002C9C)|4) -- LV1_CLEAR makes native UpdateGlobalProgress select non-festival town.
 H.warp(2,0,840,880);emu:write16(0x2002B00,99)
 local anju
 for i=0,71 do local p=0x30015A0+i*136
  if H.r32(p+4)~=0 and H.r8(p+8)==7 and H.r8(p+9)==0x45 then anju=p;break end
 end
 assert(anju,'native Anju absent')
 H.log(string.format('Anju=%08X xy=%d,%d',anju,H.r16(anju+46),H.r16(anju+50)))
 H.wait(90);local base=emu:saveStateBuffer();local spoke=false
 for _,v in ipairs({{0,16,0},{0,12,0},{0,20,0},{16,0,6},{-16,0,2},{0,-16,4}}) do
  emu:loadStateBuffer(base)
  emu:write32(H.P+44,H.r32(anju+44)+v[1]*65536);emu:write32(H.P+48,H.r32(anju+48)+v[2]*65536)
  emu:write8(H.P+20,v[3]);emu:write8(H.P+21,v[3]*4);H.wait(12);H.press(0x100);H.wait(30)
  H.log('talk offset='..v[1]..','..v[2]..' msg='..H.r8(0x2000050)..' act='..H.r8(H.P+12))
  if H.r8(0x2000050)&0x7F~=0 then spoke=true;break end
 end
 H.check(spoke,'Anju conversation actually opened');H.shot('anju_offer')
 local calls=0;local game
 emu:setBreakpoint(function() game=emu:readRegister('r0');calls=calls+1 end,0x080A1270)
 for i=1,120 do
  if calls>10 then break end
  if H.r8(0x2022809)==5 then
   if H.r8(0x2024033)~=0 then H.press(0x20) end
   H.press(1)
  elseif H.r8(0x2000050)==7 or H.r8(0x2022809)==3 then H.press(1)
  else H.wait(12,2) end
  H.wait(2)
  H.log('offer '..i..' msg='..H.r8(0x2000050)..' id='..H.r16(0x2000058)..' text='..H.r8(0x2022808)..'/'..H.r8(0x2022809)..' calls='..calls..' gameframes='..H.r32(H.PM+4))
  if i%10==0 then H.shot('offer_'..i) end
 end
 assert(game,'native Anju conversation did not start countdown')
 local before=H.r16(game+0x68);H.wait(30)
 H.check(H.r16(game+0x68)<before,'actual minigame countdown decreases with cheat OFF')
 H.action(16,12);before=H.r16(game+0x68);local startCalls=calls
 H.wait(180);H.shot('infinite_time_on')
 H.check(calls>startCalls+100 and H.r16(game+0x68)==before,'180 native countdown calls keep time exactly fixed')
 -- Place two genuine minigame Cuccos in the pen; native code must count them.
 local heap=H.r32(game+0x64)
 for i=0,1 do local p=H.r32(heap+i*4)
  assert(p>=0x30015A0 and p<0x3003BE0,'native minigame Cucco pointer')
  emu:write32(p+44,(H.r16(H.R+6)+0x368)*65536)
  emu:write32(p+48,(H.r16(H.R+8)+0x358)*65536)
 end
 H.wait(30);H.check(H.r16(game+0x6A)>=2,'native pen logic counts actual Cuccos while timer frozen')
 emu:write16(game+0x68,1);H.wait(60)
 H.check(H.r16(game+0x68)==1 and H.r8(0x2000050)&0x7F==0,'last remaining tick does not trigger minigame loss')
 H.action(16,12);H.wait(90);H.shot('time_off_results')
 H.check(H.r16(game+0x68)==0 and H.r8(0x2000050)&0x7F~=0,'OFF restores native expiry and results textbox')
 local shells=H.r16(0x2002B02)
 for i=1,90 do
  if H.r16(0x2002B02)>shells then break end
  H.press(1);H.wait(18,2)
 end
 H.shot('native_win_reward')
 H.check(H.r16(0x2002B02)>shells,'native minigame win still grants the first-level shell prize')
end)
