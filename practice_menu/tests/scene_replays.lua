local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local UI,MENU,FADE=0x2032EC0,0x2000080,0x3000FD0
local function mode() return H.r8(H.PM+H.L.sceneReplay) end
local function untilReady(f,n,label)
 for i=1,n do if f() then return end;H.wait(1) end
 error(label..string.format(' scene=%d main=%d/%d/%d ui=%d/%d menu=%d/%d room=%02X/%02X',mode(),H.r8(H.M+2),H.r8(H.M+3),H.r8(H.M+4),H.r8(UI),H.r8(UI+2),H.r8(MENU+5),H.r8(MENU+6),H.r8(H.R+4),H.r8(H.R+5)))
end
local function begin(row)
 H.menu(8,row);H.shot('warp_choices_'..row)
 H.press(1);H.check(mode()==0 and H.r8(H.PM+13)==1,'first A only arms replay '..row)
 H.shot('confirm_'..row);H.press(1)
 untilReady(function() return mode()~=0 end,250,'replay did not start')
 return emu:readRange(H.L.sceneBackup,0x4B4)
end
local function leave(save,area,room)
 H.menu(8,7);H.shot('replay_return_menu_'..mode());H.press(1)
 untilReady(function() return mode()==0 end,450,'replay return failed')
 H.check(H.r8(H.R+4)==area and H.r8(H.R+5)==room,'return to exact original room')
 H.check(emu:readRange(0x2002A40,0x4B4)==save,'all 1204 native save bytes restored')
 H.wait(35);H.shot('returned_'..area..'_'..room)
 H.press(16);H.check(H.r8(H.M+4)==2,'normal movement resumes')
end
H.run(function()
 local area,room=H.r8(H.R+4),H.r8(H.R+5)
 -- Confirmation cancelled by navigation/B, not silently retained on reopen.
 H.menu(8,5);H.press(1);H.press(128);H.press(1)
 H.check(mode()==0,'moving to other replay requires its own confirmation')
 H.press(2);H.close();H.check(mode()==0,'B cancels replay selection')
 H.option('noClip',1);H.option('freezeEnemies',1);H.option('resourceCheats',31)
 local save=begin(5)
 untilReady(function() return H.r8(UI+2)==5 and H.r8(UI)==2 and H.r8(MENU+6)==2 and H.r8(FADE)==0 end,1800,'native intro not visible')
 H.wait(65);H.shot('intro_visible')
 H.check(H.r8(H.PM+14)==0 and H.r8(H.PM+19)==0 and H.r8(H.PM+20)==0,'cheats temporarily suspended in replay')
 H.menu(15,1);local before=emu:readRange(0x2002A40,0x4B4);H.press(1)
 H.check(emu:readRange(0x2002A40,0x4B4)==before,'unrelated menu mutations blocked during replay')
 leave(save,area,room)
 H.check(H.r8(H.PM+14)==1 and H.r8(H.PM+19)==1 and H.r8(H.PM+20)==31,'cheat settings restored')
 H.option('noClip',0);H.option('freezeEnemies',0);H.option('resourceCheats',0)
 H.warp(0x22,0x11,120,100);area,room=0x22,0x11
 save=begin(6)
 untilReady(function() return H.r8(H.R+4)==0x89 and H.r8(H.R+5)==1 and H.r8(FADE)==0 and H.r8(H.S+0x8B)~=0 end,1800,'native ending not running')
 untilReady(function() return H.r8(0x2000050)&0x7F~=0 end,2000,'ending dialogue not reached')
 H.wait(40);H.shot('ending_visible');leave(save,area,room)
 -- The new execution rows remain blocked in the native inventory overlay.
 H.press(8);H.wait(80);H.menu(8,5);H.press(1);H.press(1)
 H.check(mode()==0,'intro warp blocked from native inventory')
 H.menu(8,6);H.press(1);H.press(1);H.check(mode()==0,'ending warp blocked from native inventory')
end)
