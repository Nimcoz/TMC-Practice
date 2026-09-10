local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.warp(0x22,0x11,64,92)
 local house=emu:saveStateBuffer();local spoke=false
 for i=0,71 do local p=0x30015A0+i*136
  if H.r32(p+4)~=0 and H.r8(p+8)==7 then
   emu:loadStateBuffer(house);emu:write32(H.P+44,H.r32(p+44));emu:write32(H.P+48,H.r32(p+48)+20*65536)
   emu:write8(H.P+20,0);H.wait(6);H.press(0x100);H.wait(90)
   if H.r8(0x2000050)&0x7F~=0 then spoke=true;break end
  end
 end
 if not spoke then H.warp(3,1,336,272);H.wait(200);spoke=H.r8(0x2000050)&0x7F~=0 end
 H.check(spoke,'actual native textbox is open before Break Free');H.shot('textbox_before')
 H.action(16,2);H.wait(60);H.shot('textbox_after')
 H.log('message='..H.r8(0x2000050)..' ctrl='..H.r8(H.S+139)..' status='..emu:readRange(H.PM+H.L.status,28))
 H.check(H.r8(0x2000050)&0x7F==0,'native closing state machine removes textbox')
 H.check(H.r8(H.S+139)==0,'control lock released after textbox cleanup')
 local x,y=H.r32(H.P+44),H.r32(H.P+48);H.wait(24,0x20)
 H.check(x~=H.r32(H.P+44) or y~=H.r32(H.P+48),'controller moves Link after textbox Break Free')
 H.warp(3,1,336,272);H.wait(30);H.action(16,2);H.wait(60)
 H.check(H.r8(H.S+139)==0,'scripted intro control released')
 H.shot('cutscene_after')
end)
