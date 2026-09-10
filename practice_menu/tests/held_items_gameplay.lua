local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.action(14,0);H.warp(0x70,5,120,80)
 local house=emu:saveStateBuffer();local pots={}
 for i=0,71 do local e=0x30015A0+i*0x88;if H.r32(e+4)~=0 and H.r8(e+8)==6 and H.r8(e+9)==5 then pots[#pots+1]={x=H.r32(e+44),y=H.r32(e+48),e=e} end end
 H.log('native pot entities='..#pots)
 local lifted=false
 for _,pot in ipairs(pots) do for _,gap in ipairs({16,12,8,20,24}) do if not lifted then
  emu:loadStateBuffer(house);emu:write32(H.P+44,pot.x);emu:write32(H.P+48,pot.y+gap*65536);emu:write8(H.P+20,0)
  H.wait(6,0x40);H.wait(4);H.wait(20,0x100);H.wait(16,0x180);H.wait(4)
  H.log(string.format('PICKUP_TRIAL pot=%08X gap=%d held=%d action=%d state=%d',pot.e,gap,H.r8(H.S+5),H.r8(H.P+12),H.r8(H.S+168)))
  if H.r8(H.S+5)==4 then lifted=true;H.log('NATIVE_PICKUP pot='..pot.e..' held='..H.r8(H.S+5)) end
 end end end
 H.check(lifted,'native R input lifts a real pot')
 if lifted then
  H.shot('pot_held');H.action(17,1);H.wait(10)
  H.check(H.r8(H.S+5)==0 and H.r16(0x2002AF4)==0,'Delete All clears real carried-item and equipped states')
  local x=H.r32(H.P+44);H.wait(12,0x10);H.check(H.r32(H.P+44)~=x,'Link can move after deleting inventory while carrying')
  H.shot('pot_after_delete')
 end
 emu:loadStateBuffer(house);emu:write8(0x2002AF4,17);H.wait(30,1)
 H.check(H.r8(H.S+28)~=0,'native Gust Jar is active before Delete All')
 H.action(17,1);H.wait(8)
 H.check(H.r8(H.S+28)==0 and H.r16(0x2002AF4)==0,'Delete All cleans native Gust Jar state')
end)
