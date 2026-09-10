local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.menu(13,4);H.press(0x10);H.press(0x10)
 H.menu(13,5);H.press(1);H.wait(60);H.shot('settings_saved')
 H.check(emu:readRange(H.PM+H.L.status,28):find('MENU SETTINGS SAVED')~=nil,'both settings records written/read back')
 H.close();emu:reset();H.boot()
 H.check(H.r8(H.PM+H.L.menuTheme)==2,'theme survives reset without native game save')
 H.menu(13,5);H.shot('settings_reloaded');H.close()
 H.warp(0x23,7,120,120)
 H.menu(16,9);H.press(1);H.press(0x40);H.press(1);H.close()
 local viewer
 for i=0,71 do local e=0x30015A0+i*0x88
  if H.r32(e+4)~=0 and H.r8(e+8)==6 and H.r8(e+9)==0x22 and H.r8(e+10)==0 then viewer=e end
 end
 assert(viewer,'no viewer')
 emu:write32(H.P+44,H.r32(viewer+44));emu:write32(H.P+48,H.r32(viewer+48)+20*65536);emu:write8(H.P+20,0)
 H.wait(8);H.press(0x100);H.wait(80)
 local fig=H.r32(0x80A46B4)+0x1C
 H.log('FIXTURE select figure130 through native menu index '..string.format('%08X',fig))
 emu:write8(fig,130);H.wait(5)
 H.press(0x80);H.wait(12);H.shot('figure_131')
 H.check(H.r8(fig)==131,'controller crosses old130 boundary')
 for i=1,5 do H.press(0x80);H.wait(20) end
 H.wait(15);H.shot('figure_136')
 H.check(H.r8(fig)==136,'controller reaches136 before credits')
 H.press(0x80);H.wait(20);H.check(H.r8(fig)==136,'native list clamps at136')
 H.check(H.r8(0x2002A46)==0,'credits flag remains untouched')
 H.press(2);H.wait(80);H.check(H.r8(H.M+4)==2,'expanded viewer closes normally')
end)
