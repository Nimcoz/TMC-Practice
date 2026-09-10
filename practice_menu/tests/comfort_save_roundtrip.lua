local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function row(p,r) H.menu(p,r);H.press(1) end
local function saveGame()
 H.press(8);H.wait(65);assert(H.r8(H.M+4)==7,'native inventory did not open')
 emu:write8(0x2000083,16);H.wait(5);H.press(1);H.wait(60)
 H.press(0x40);H.press(1);H.wait(180)
 H.press(1);H.wait(60);H.press(8);H.wait(90)
 -- Depending on the native post-save screen this START opened inventory;
 -- return through its normal controller path before opening Practice Menu.
 if H.r8(H.M+4)==7 then H.press(8);H.wait(90) end
 assert(H.r8(H.M+4)==2,'native save did not return to gameplay')
end
H.run(function()
 local name=emu:readRange(0x2002AC0,6);local max=H.r8(0x2002AEB);local figures=H.r8(0x2002AF0)
 H.menu(8,3);H.option('selectedArea',3);H.option('selectedRoom',1);H.press(1)
 row(13,2);row(24,6);row(24,7) -- L+SELECT open
 row(13,6);row(24,5);row(24,6);row(24,4);row(24,7) -- START+A confirm
 row(13,5);H.wait(65);H.close()
 row(16,10);H.press(0x80);H.press(9);H.wait(220);H.close()
 saveGame();H.shot('native_saved_completion_with_custom_keys')
 H.check(H.r8(H.PM+H.L.completionUndoValid)==1,'native save without reload preserves undo backup')
 row(16,11);H.press(0x80);H.press(9);H.wait(220);H.close();saveGame()
 H.shot('native_saved_undo');emu:reset();H.boot()
 H.check(H.r8(0x2002A46)==0 and H.r8(0x2002AEB)==max and H.r8(0x2002AF0)==figures,'undo persisted through native save/reset/load')
 H.check(emu:readRange(0x2002AC0,6)==name,'native undo roundtrip preserves player identity')
 H.check(H.hotkey()==0x204 and H.r16(H.PM+H.L.confirmHotkey)==9,'native save keeps both custom hotkeys')
 H.check(H.r8(H.PM+H.L.favorites)==3 and H.r8(H.PM+H.L.favorites+1)==1,'native save keeps favorite destination')
 H.check(H.r8(H.PM+H.L.completionUndoValid)==0,'undo backup is never serialized to EEPROM')
 H.menu(13,2);H.shot('custom_setup_after_native_save');H.close()
end)
