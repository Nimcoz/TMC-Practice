local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.warp(0x23,7,120,120)
 H.menu(16,9);H.press(1);H.press(0x40);H.press(1);H.close()
 local device,viewer
 for i=0,71 do local e=H.P+0x440+i*0x88
  if H.r32(e+4)~=0 and H.r8(e+8)==6 and H.r8(e+9)==0x22 then
   H.log('native device type='..H.r8(e+10)..' action='..H.r8(e+12)..' count='..H.r8(e+128))
   if H.r8(e+10)==3 then device=e end
   if H.r8(e+10)==0 then viewer=e end
  end
 end
 H.check(device~=nil and H.r8(device+128)==136,'already-loaded Carlov dispenser refreshed to 136')
 H.check(device~=nil and H.r8(device+130)==0 and H.r8(device+131)==0,'already-loaded chance cache is zero, not negative')
 if viewer then
  H.log('FIXTURE position below native collection viewer')
  emu:write32(H.P+44,H.r32(viewer+44));emu:write32(H.P+48,H.r32(viewer+48)+20*65536);emu:write8(H.P+20,0)
  H.wait(8);H.press(0x100);H.wait(80);H.shot('native_figurine_viewer')
  H.check(H.r8(H.M+4)==7 and H.r8(H.PM+13)==0,'native collection viewer opens through controller interaction')
  H.press(0x80);H.press(0x10);H.wait(15);H.shot('native_figurine_description')
  H.press(2);H.wait(80);H.check(H.r8(H.M+4)==2,'native viewer closes to playable room')
 else H.check(false,'native collection viewer exists') end
end)
