local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function row(p,r,k) H.menu(p,r);H.press(k or 1) end
local function fav(i) return H.PM+H.L.favorites+i*2 end
H.run(function()
 H.menu(8,3);H.option('selectedArea',3);H.option('selectedRoom',1)
 local before=emu:readRange(0x2002A40,0x4B4)
 H.press(1);H.check(H.r8(fav(0))==3 and H.r8(fav(0)+1)==1,'add named favorite')
 H.press(1);H.check(H.r8(fav(0))==255 and H.r8(fav(0)+1)==255,'toggle removes favorite without duplicates')
 H.press(1);row(8,4);H.shot('favorites_named')
 H.check(H.r8(H.PM+H.L.page)==23,'favorite list opens')
 row(23,1);H.check(H.r8(H.PM+H.L.page)==23,'empty favorite cannot warp')
 row(23,0);H.shot('favorite_preview')
 H.check(H.r8(H.PM+H.L.page)==8 and H.r8(H.PM+H.L.cursor+8)==2,'favorite first opens named destination preview')
 H.check(emu:readRange(0x2002A40,0x4B4)==before,'favorite edits do not mutate game save RAM')
 -- Fill the remaining slots using only verified native room headers.
 local areas={0,1,2,4,5,6,7};for _,a in ipairs(areas) do
  H.menu(8,3);H.option('selectedArea',a);H.option('selectedRoom',0);H.press(1)
 end
 local count=0;for i=0,7 do if H.r8(fav(i))~=255 then count=count+1 end end
 H.check(count==8,'eight distinct native room favorites fit')
 local full=emu:readRange(fav(0),16)
 H.menu(8,3);H.option('selectedArea',0x23);H.option('selectedRoom',7);H.press(1)
 H.check(emu:readRange(fav(0),16)==full,'ninth favorite does not overwrite an existing slot')
 H.shot('favorites_full');row(8,4);H.shot('favorites_eight')
 row(23,0);H.press(1);H.wait(180);H.close()
 H.check(H.r8(H.R+4)==3 and H.r8(H.R+5)==1,'favorite warp uses existing native transition')
 -- Open the editor normally, then choose L+SELECT (remove R).
 row(13,2);row(24,6);H.shot('hotkey_draft');row(24,7)
 H.check(H.hotkey()==0x204,'menu hotkey changed through editor to L+SELECT')
 H.close();H.menu(2,1);H.shot('custom_hotkey_player');H.close()
 H.menu(13,2);H.press(1) -- open menu-hotkey editor
 row(24,8);row(24,3);row(24,5);row(24,6) -- empty draft
 row(24,7);H.check(H.hotkey()==0x204 and H.r8(H.PM+H.L.page)==24,'empty binding rejected without losing working hotkey')
 row(24,1);row(24,7);H.check(H.hotkey()==0x204,'single A menu binding rejected')
 H.shot('hotkey_invalid');H.press(2)
 -- Confirmation: START+A, from default L+R+A.
 row(13,6);row(24,5);row(24,6);row(24,4);row(24,7)
 H.check(H.r16(H.PM+H.L.confirmHotkey)==9,'confirmation changed to START+A')
 -- Reject opening chord A+START because it would close the confirmation.
 row(13,2);row(24,5);row(24,3);row(24,1);row(24,4);row(24,7)
 H.check(H.hotkey()==0x204 and H.r8(H.PM+H.L.page)==24,'menu/confirmation overlap rejected')
 H.press(2)
 row(13,5);H.wait(65);H.shot('comfort_settings_saved');H.close()
 emu:reset();H.boot()
 H.check(H.hotkey()==0x204 and H.r16(H.PM+H.L.confirmHotkey)==9,'both custom combinations survive real reset')
 H.check(emu:readRange(fav(0),16)==full,'eight favorites survive real reset')
 H.menu(2,1);H.close();H.press(H.hotkey());H.wait(35)
 H.check(H.r8(H.PM+H.L.page)==2 and H.r8(H.PM+H.L.cursor+2)==1,'custom hotkey preserves last page and cursor')
 H.close()
 -- Destructive action still needs its configured confirmation, A alone does nothing.
 row(16,10);H.press(0x80);before=emu:readRange(0x2002A40,0x4B4);H.press(1)
 H.check(emu:readRange(0x2002A40,0x4B4)==before,'A alone cannot confirm after rebinding')
 H.shot('custom_completion_confirmation');H.press(9);H.wait(220);H.close()
 H.check(H.r8(0x2002A46)==1 and H.r8(H.PM+H.L.completionUndoValid)==1,'custom START+A applies completion and captures undo')
 H.menu(16,11);H.press(1);H.shot('custom_undo_confirmation');H.press(0x80);H.press(9);H.wait(220);H.close()
 H.check(H.r8(0x2002A46)==0 and H.r8(H.PM+H.L.completionUndoValid)==0,'custom START+A restores precompletion gameplay')
end)
