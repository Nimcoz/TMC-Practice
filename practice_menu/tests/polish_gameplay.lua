local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 local save=0x2002A40
 local baseline=emu:saveStateBuffer()
 -- Themes: hardware palette, page memory, native palette restoration.
 local palette=emu:readRange(0x5000000,512)
 for theme=1,4 do
  H.menu(13,4);H.press(1);H.shot('theme_'..(theme%4))
  H.check(H.r8(H.PM+H.L.menuTheme)==theme%4,'theme cycles '..theme%4)
  H.close();H.check(emu:readRange(0x5000000,512)==palette,'native palette restored theme '..theme%4)
 end
 -- Collectibles are confirmation-gated and preserve unrelated save fields.
 local kin=emu:readRange(save+0x114,0x148)
 H.menu(16,8);H.press(1);H.shot('fragments_confirm')
 H.check(H.r8(H.PM+H.L.cursor+14)==1,'fragments default CANCEL')
 H.press(1);H.check(emu:readRange(save+0x114,0x148)==kin,'cancel has no bag writes')
 H.menu(16,8);H.press(1);H.press(0x40);H.press(1)
 local bag=emu:readRange(save+0x114,0x148)
 local ok=true;for i=0,16 do if H.r8(save+0x118+i)~=0x65+i or H.r8(save+0x12B+i)~=99 then ok=false end end
 H.check(ok and H.item(103)==1,'all 17 fragment types x99 and bag ownership')
 H.check(bag:sub(1,4)==kin:sub(1,4) and bag:sub(43)==kin:sub(43),'fusion history/counters untouched')
 -- Independent native ROM literal, not the module's struct descriptor.
 local figureBase=H.r32(0x8088398)
 H.check(figureBase==save+0xCE,'native ReadBit literal confirms actual figure bitset address')
 local figs=emu:readRange(figureBase,36)
 local before=emu:readRange(save+0x25C,512)
 H.menu(16,9);H.press(1);H.shot('figurines_confirm');H.press(0x40);H.press(1);H.close()
 ok=true;for i=1,136 do if (H.r8(figureBase+(i>>3))>>(i&7))&1~=1 then ok=false end end
 H.check(ok and H.r8(save+0xB0)==136 and H.r8(save+0xBB)==1 and H.item(62)==1,'136 owned figures, native count/completion and medal')
 H.check(emu:readRange(figureBase+18,18)==figs:sub(19),'unused figurine bytes untouched')
 local changes={};for i=0,511 do local a=before:byte(i+1);local b=H.r8(save+0x25C+i);if a~=b then changes[i]=a~b end end
 for i,v in pairs(changes) do H.check((i==0xB and v==2) or (i==0x4B and (v&~0xC0)==0),'only collection completion/viewer flag changed '..i) end
 H.warp(0x23,7,120,120);H.wait(90)
 H.check(H.r8(save+9)==136,'native Carlov init counts owned figures as available without beating story')
 H.shot('carlov_complete')
 H.press(8);H.wait(40);H.shot('native_inventory_collected');H.press(2);H.wait(40)
 -- Independent native scene fixture tests later use an unchanged state.
 emu:loadStateBuffer(baseline);H.wait(3)
 H.menu(11,2);H.press(1);H.shot('general_flags')
 local beforeBit=H.r8(save+0x25C+2)
 H.press(1);H.check(H.r8(save+0x25C+2)==(beforeBit~8),'named MET ZELDA edits exactly global bit 13')
 H.press(1);H.check(H.r8(save+0x25C+2)==beforeBit,'named toggle reversible')
 H.menu(11,4);H.press(1);H.shot('wind_portals')
 local wind=H.r32(save+0x40);H.press(1)
 H.check(H.r32(save+0x40)==(wind~0x1000000),'named Crenel portal edits exactly windcrest bit24')
 H.press(1)
 H.menu(11,5);H.press(1);H.shot('edit_history')
 H.check(H.r8(H.PM+H.L.flagLogCount)==4,'history records four actual edits')
 H.close()
 H.warp(0x48,0,120,120);H.menu(11,3);H.press(1);H.shot('dungeon_flags')
 local d=H.r8(H.PM+H.L.menuDungeonIndex);H.log('native dungeon index '..d)
 if d<16 then
  local k=H.r8(save+0x45C+d);H.press(0x10);H.check(H.r8(save+0x45C+d)==k+1,'native current dungeon small key +1')
  H.press(0x20);H.check(H.r8(save+0x45C+d)==k,'small key -1')
  for row=1,3 do
   emu:write8(H.PM+H.L.cursor+10,row);local item=H.r8(save+0x46C+d);H.press(1)
   H.check(H.r8(save+0x46C+d)==(item~(1<<(row-1))),'native map/compass/big key bit '..row)
   H.press(1)
  end
 else H.check(false,'fixture must be native dungeon') end
 H.close()
end)
