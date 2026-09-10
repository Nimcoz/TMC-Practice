local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function selected() return H.r32(H.PM+H.L.actorSelected) end
local function mark(e) return H.PM+H.L.actorMarks+(e-0x30015A0)//0x88 end
local function choose(e)
 H.menu(27,0);H.option('actorFilter',0)
 local index=0
 for i=0,111 do
  local p=i<80 and (H.P+i*0x88) or (0x2033290+(i-80)*0x40)
  local prev=H.r32(p)
  if prev>0 and prev<0x80000000 and H.r32(p+4)~=0 and (H.r8(p+16)&16)==0 then
   if p==e then emu:write8(H.PM+H.L.cursor+27,index);H.wait(2);H.press(1);return end
   index=index+1
  end
 end
 error('actor not in native room list')
end
local function spawn(i)
 H.menu(29,1);H.option('actorSpawn',i);H.press(1);H.wait(90);H.close()
 local p=selected()
 H.check(p>=0x30015A0 and p<0x3003BE0,'spawn '..i..' returns live native pool actor')
 assert(p>=0x30015A0 and p<0x3003BE0,'spawn blocked: '..emu:readRange(H.PM+H.L.status,28))
 return p
end
local function remove(p)
 choose(p);H.menu(28,1);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==30 and H.r8(H.PM+H.L.cursor+30)==0,'remove opens default CANCEL')
 H.press(0x80);H.press(1)
 H.check(H.r32(p+4)~=0 and H.r8(H.PM+13)==1,'plain A cannot remove')
 H.shot('remove_confirmation');H.press(0x301);H.wait(45);H.close()
 H.check(selected()==0 and H.r8(mark(p))==0,'native deletion invalidates selection and per-slot marks')
end
H.run(function()
 H.menu(0,10);H.press(1);H.check(H.r8(H.PM+H.L.page)==26,'Actors accessible from root with native input');H.shot('actors_home')
 H.press(1);H.shot('actors_room_list')
 H.check(H.r8(H.PM+H.L.page)==27,'room list opens')
 H.press(0x20);H.shot('actors_managers');H.check(H.r8(H.PM+H.L.actorFilter)==6,'filter wraps left to managers')
 H.press(0x10);H.check(H.r8(H.PM+H.L.actorFilter)==0,'filter wraps right to ALL')
 choose(H.P);H.shot('link_readonly');H.press(1);H.menu(28,1);H.press(1)
 H.check(H.r8(H.PM+H.L.page)==30 and H.r32(H.P+4)~=0,'Link removal allowed but requires confirmation')
 H.press(1);H.menu(28,0);H.press(1) -- Cancel removal, then unfreeze Link.
 H.close();H.warp(0,0,0x1F8,0x1F8);H.openFloor()
 -- Supplied save has no sword: only protect test player from spawned contact.
 emu:write8(H.PM+16,1)
 local beforeFlags=emu:readRange(0x2002C9C,512)
 local e=spawn(0);H.check(H.r8(e+8)==3 and H.r8(e+9)==0 and H.r8(e+12)>0,'Octorok initialized through native engine')
 choose(e);H.shot('octorok_details');H.press(1);H.close();H.wait(12)
 local x,y,t=H.r32(e+44),H.r32(e+48),H.r8(e+14)
 H.wait(100);H.shot('octorok_frozen_ingame')
 H.check(H.r32(e+44)==x and H.r32(e+48)==y and H.r8(e+14)==t and (H.r8(mark(e))&1)==1,'individual AI freeze stable100 frames')
 choose(e);H.press(1);H.close()
 local changed=false
 for i=1,100 do H.wait(1);if H.r32(e+44)~=x or H.r32(e+48)~=y or H.r8(e+14)~=t then changed=true end end
 H.check(changed,'unfreeze resumes native AI')
 remove(e)
 H.check(emu:readRange(0x2002C9C,512)==beforeFlags,'spawn/remove do not write story/local completion flags')
 H.openFloor();local e2=spawn(0)
 H.check((H.r8(mark(e2))&1)==0,'new allocation never inherits removed actor freeze')
 choose(e2);H.press(1);H.close();H.warp(0,0,0x1F8,0x1F8)
 H.check(selected()==0 and emu:readRange(H.PM+H.L.actorMarks,72)==string.rep('\0',72),'same-room native reload clears all actor handles/freezes')
 for i=1,4 do
  H.openFloor();emu:write8(H.P+20,2)
  local p=spawn(i);H.shot('spawn_'..i..'_ingame');choose(p);H.shot('spawn_'..i..'_details');H.close()
  if i<3 then
   H.check(H.r8(p+8)==3 and H.r8(p+9)==(i==1 and 8 or 0x2C) and H.r8(p+12)>0,'native enemy variant '..i..' initialized')
   remove(p)
  else
   H.check(H.r8(p+8)==6 and H.r8(p+9)==0 and H.r8(p+10)==(i==3 and 0x5F or 0x54),'native pickup variant '..i)
   local health,rupees=H.r8(0x2002AEA),H.r16(0x2002B00)
   if i==3 then emu:write8(0x2002AEA,8);health=8 end
   emu:write32(H.P+44,H.r32(p+44));emu:write32(H.P+48,H.r32(p+48));H.wait(90)
   H.check(i==3 and H.r8(0x2002AEA)>health or i==4 and H.r16(0x2002B00)==rupees+1,'pickup produces native health/rupee effect '..i)
   H.check(selected()==0,'native pickup collection invalidates selection '..i)
  end
  H.warp(0,0,0x1F8,0x1F8)
 end
 H.menu(29,0);H.shot('curated_spawner');H.close()
end)
