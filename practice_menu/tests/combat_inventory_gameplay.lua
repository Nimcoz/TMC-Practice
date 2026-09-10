local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.action(14,0)
 local initial=emu:saveStateBuffer()
 emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20);H.log('FIXTURE TABIDACHI enemy spawn gate only, no release/save change')
 H.warp(0,0,504,504);H.option('freezeEnemies',1)
 local enemy
 for i=0,71 do local e=0x30015A0+i*0x88;if H.r8(e+8)==3 and H.r8(e+9)==0 and H.r32(e+4)~=0 then enemy=e;break end end
 assert(enemy,'native Octorok not found')
 local ex,ey=H.r32(enemy+44),H.r32(enemy+48);H.wait(60)
 H.check(H.r32(enemy+44)==ex and H.r32(enemy+48)==ey,'Enemy Freeze stops real Octorok AI movement')
 local hp=H.r8(0x2002AEA)
 emu:write32(H.P+44,ex);emu:write32(H.P+48,ey);H.log('FIXTURE overlap Link with native enemy for real contact')
 H.wait(8)
 H.check(H.r8(0x2002AEA)<hp and H.r8(H.P+66)~=0,'frozen enemy still causes real contact damage / knockback')
 emu:write8(H.PM+16,1);H.wait(70)
 emu:write32(H.P+44,H.r32(enemy+44));emu:write32(H.P+48,H.r32(enemy+48)+20*65536);emu:write8(H.P+20,0);H.wait(5)
 H.shot('enemy_before_hit');local damaged,knock,removed=false,false,false
 for t=1,220 do
  H.wait(1,t%30<3 and 1 or 0)
  if H.r8(enemy+8)==3 and H.r8(enemy+9)==0 then
   if H.r8(enemy+69)==0 then damaged=true end
   if H.r8(enemy+66)~=0 then knock=true end
  end
  if H.r32(enemy+4)==0 or H.r8(enemy+8)~=3 or H.r8(enemy+9)~=0 then removed=true end
 end
 H.check(damaged and knock and removed,'frozen enemy takes sword damage, native knockback, dies and is removed')
 H.check(H.r8(0x3003DC0)==0,'Enemy Freeze did not set global event priority');H.shot('enemy_after_death')
 -- Native queued freeze entry (synthetic collision precondition, not an action patch).
 H.option('freezeEnemies',0);H.openFloor();local ground=emu:saveStateBuffer()
 emu:write32(H.S+48,H.r32(H.S+48)|0x801);emu:write8(H.S+12,13);emu:write8(H.S+20,1)
 H.log('FIXTURE native frozen queued action and native PL_FROZEN/PL_BUSY collision flags')
 H.wait(5);H.check(H.r8(H.P+12)==13 and H.r8(H.P+13)==1,'native frozen initialization ran')
 H.action(16,2);H.check(H.r8(H.P+12)==1 and (H.r32(H.S+48)&0x801)==0 and (H.r8(H.P+16)&0x80)~=0,'Break Free clears native freeze flags and restores collision/control')
 -- Real clone tiles, all elements independently removed, native tutorial gates in test RAM only.
 emu:loadStateBuffer(initial);emu:write8(0x2002D0B,H.r8(0x2002D0B)|3)
 H.log('FIXTURE native Sanctuary 378/379 script gates; NO save or release patch')
 H.menu(15,0);for i=0,3 do if H.item(64+i)~=0 then emu:write8(H.PM+H.L.cursor+15,i);H.press(1) end end;H.close()
 H.warp(0x78,1,312,600)
 local ptr=H.r32(0x800027C);local tiles={};local w,h=H.r16(H.R+30),H.r16(H.R+32)
 for y=0,math.floor(h/16)-1 do for x=0,math.floor(w/16)-1 do
  if H.r8(ptr+y*64+x)==87 then tiles[#tiles+1]={x=H.r16(H.R+6)+x*16+8,y=H.r16(H.R+8)+y*16+8} end
 end end
 assert(#tiles>=4,'four native clone tiles not found')
 emu:write32(H.P+44,tiles[1].x*65536);emu:write32(H.P+48,tiles[1].y*65536);H.log('FIXTURE Link on native clone plate; no clone/action injection');H.wait(15)
 H.wait(280,1);H.check(H.r8(H.S+160)==4 and H.r32(0x3004040)~=0,'Four Sword charges a native clone with no elements')
 local three=false;local activated=false
 for t=1,115 do
  H.wait(1,0x11)
  if H.r32(0x3004040)~=0 and H.r32(0x3004044)~=0 and H.r32(0x3004048)~=0 then three=true end
  if (H.r32(H.S+48)&0x400000)~=0 then activated=true end
  if t%10==0 then H.log(string.format('CLONE x=%d state=%d flags=%08X ptrs=%08X,%08X,%08X',H.r16(H.P+46),H.r8(H.S+160),H.r32(H.S+48),H.r32(0x3004040),H.r32(0x3004044),H.r32(0x3004048))) end
 end
 H.check(three and activated,'three native clones activate after visiting four real plates');H.shot('three_clones')
 H.action(17,1)
 H.check(H.r32(0x3004040)==0 and H.r32(0x3004044)==0 and H.r32(0x3004048)==0 and (H.r32(H.S+48)&0x400000)==0,'Delete All removes active clones and cloning flag')
 H.check(H.r8(0x2002B32)==1 and H.r16(0x2002AF4)==0 and H.r16(H.S+172)==0,'Delete All preserves native menu sentinel only; equipment and skills clear')
 H.press(8);H.wait(55);H.check(H.r8(H.M+4)==7 and H.r8(H.PM+13)==0,'native pause/save stays available after Delete All');H.press(8);H.wait(60)
 -- Downgrade through every valid sword while charged; native cleanup must unwind it.
 emu:loadStateBuffer(ground);H.wait(4)
 for _,id in ipairs({4,3,2,1,0}) do
  H.wait(160,1);H.menu(4,0);H.press(0x20);H.close()
  local actual=0;for i=1,6 do if H.item(i)~=0 then actual=i end end
  H.check(actual==id and H.r8(H.S+160)<3 and H.r8(H.S+27)==0,'charged sword downgrade to '..id..' cleans active item')
 end
end)
