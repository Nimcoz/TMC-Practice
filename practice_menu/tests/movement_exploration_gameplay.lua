local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.action(14,0);H.warp(0,0,504,504);H.openFloor()
 local base=emu:saveStateBuffer()
 -- Native roll / Roc's Cape trajectories must not be multiplied by Walk Speed.
 for _,case in ipairs({'roll','cape'}) do
  local samples={}
  local testBase=base
  if case=='roll' then
   emu:loadStateBuffer(base);H.wait(6,0x10);H.wait(3,0x110)
   H.log('ROLL fixture action='..H.r8(H.P+12)..' queued='..H.r8(H.S+12));testBase=emu:saveStateBuffer()
  end
  for mode=0,2,2 do
   emu:loadStateBuffer(testBase);emu:setKeys(0);H.wait(2);H.option('speedMode',mode)
   if case=='cape' then emu:write8(0x2002AF4,20);H.log('FIXTURE equipped owned Rocs Cape') end
   if case=='cape' then H.wait(3,1) end
   samples[mode]={}
   for t=1,45 do
    H.wait(1,case=='roll' and 0x10 or 0)
    samples[mode][t]={x=H.r32(H.P+44),y=H.r32(H.P+48),z=H.r32(H.P+52),action=H.r8(H.P+12),jump=H.r8(H.S+2)}
   end
  end
  local active,matched=0,true
  for t=1,45 do
   local a,b=samples[0][t],samples[2][t]
   if (case=='roll' and a.action==24) or (case=='cape' and (a.z~=0 or a.jump~=0)) then
    active=active+1;if a.x~=b.x or a.y~=b.y or a.z~=b.z then matched=false end
   end
  end
  H.check(active>5 and matched,'Walk Speed leaves native '..case..' trajectory unchanged ('..active..' active frames)')
 end
 emu:loadStateBuffer(base);H.wait(4);emu:write8(0x2002AF4,20);H.wait(8,1)
 H.check(H.r32(H.P+52)~=0,'real cape jump before Ground Reset')
 H.action(9,6);H.log('GROUND status '..emu:readRange(H.PM+H.L.status,28))
 H.check(H.r32(H.P+52)==0 and H.r32(H.P+32)==0 and H.r8(H.S+2)==0,'Ground Reset on verified floor removes native jump Z / velocity / state')
 emu:loadStateBuffer(base);H.wait(4);H.wait(220,1)
 H.check(H.r8(H.S+160)>=3,'native charged sword before Break Free')
 H.action(16,2);H.check(H.r8(H.S+160)<3 and H.r8(H.S+27)==0 and H.r8(H.P+12)==1,'Break Free cleans charged sword through native reset')
 local x=H.r32(H.P+44);H.wait(12,0x10);H.check(H.r32(H.P+44)~=x,'movement resumes after Break Free')
 -- Native scene: explicit Break Free now releases the control lock once.
 emu:loadStateBuffer(base);H.warp(3,1,336,272)
 H.menu(16,2);H.press(1)
 H.close()
 H.check(H.r8(H.PM+13)==0,'Break Free can be requested from native scene')
 -- Exact known exploration fixtures from the sealed baseline.
 emu:loadStateBuffer(base);H.option('noClip',0);H.warp(0x80,0,216,256)
 local castle=emu:saveStateBuffer();local endx={}
 for on=0,1 do
  emu:loadStateBuffer(castle);emu:setKeys(0);H.wait(3);H.option('noClip',on)
  local reversals=0;local old=H.r32(H.P+44);local room=H.r8(H.R+5);local maxx=0;local transitions=0
  for t=1,260 do
   H.wait(1,0x10);local now=H.r32(H.P+44)
   if H.r8(H.R+5)==0 and H.r8(H.R+4)==0x80 then maxx=math.max(maxx,H.r16(H.P+46)) end
   if H.r8(H.R+5)==room and now<old then reversals=reversals+1 end
   if H.r8(H.R+5)~=room then transitions=transitions+1;H.log('CASTLE native room '..room..' -> '..H.r8(H.R+5)..' area='..H.r8(H.R+4)) end
   old=now;room=H.r8(H.R+5)
  end
  endx[on]=maxx;H.log('CASTLE on='..on..' max room0 x='..maxx..' finalx='..H.r16(H.P+46)..' reversals='..reversals..' transitions='..transitions);H.shot('castle_'..on)
  if on==1 then H.check(reversals==0 and endx[on]>560,'castle geometry crossed without backwards writer') end
 end
 H.check(endx[0]<endx[1],'No Clip OFF restores native castle obstruction')
 for _,entry in ipairs({{0x70,0x17,'sky'},{0x58,3,'pit'}}) do
  H.option('noClip',1)
  local hdr=H.r32(0x811E214+entry[1]*4)+entry[2]*10
  H.warp(entry[1],entry[2],math.floor(H.r16(hdr+4)/2),math.floor(H.r16(hdr+6)/2))
  local pitbase=emu:saveStateBuffer();local fall=false;local pit=false;local sx=H.r16(H.P+46);local moved=0
  for t=1,100 do
   H.wait(1,0x10);local a=H.r8(H.P+12)
   if a==3 or a==26 then fall=true end
   if H.r8(H.S+18)==1 then pit=true end
   if H.r8(H.R+5)==entry[2] then moved=math.max(moved,math.abs(H.r16(H.P+46)-sx)) end
  end
  H.log(entry[3]..' crossed='..moved..' pit_seen='..tostring(pit)..' fall='..tostring(fall));H.shot(entry[3]..'_on')
  H.check(pit and not fall and moved>45,entry[3]..' void remains walkable without fall / respawn')
  emu:loadStateBuffer(pitbase);H.option('noClip',0);fall=false
  for t=1,40 do H.wait(1,0x10);local a=H.r8(H.P+12);if a==3 or a==26 then fall=true end end
  H.check(fall,entry[3]..' No Clip OFF restores native falling')
  emu:loadStateBuffer(pitbase);H.wait(3);H.action(9,6)
  H.log(entry[3]..' reset_status '..emu:readRange(H.PM+H.L.status,28))
  H.check(emu:readRange(H.PM+H.L.status,28):find('UNSAFE')~=nil,'Ground Reset refuses '..entry[3]..' void')
 end
 H.option('noClip',1);H.warp(6,2,216,400)
 -- Give All now also grants element-clear flags. Complete Ezlo's native
 -- post-Fire dialogue by controller before measuring collision; no flag edits
 -- or bypass of its native control lock.
 if H.r8(0x2000050)&0x7F~=0 then
  H.log('CLIFF native post-element dialogue before movement');H.shot('cliff_native_dialogue')
  for t=1,720 do
   if H.r8(0x2000050)&0x7F~=0 and t%12==0 then H.press(1) else H.wait(1) end
   if H.r8(0x2000050)&0x7F==0 and H.r8(H.S+139)==0 and H.r8(0x3003DC0)==0 then break end
  end
 end
 H.check(H.r8(0x2000050)&0x7F==0 and H.r8(H.S+139)==0 and H.r8(0x3003DC0)==0,'native Crenel dialogue ends normally before cliff test')
 local sy=H.r16(H.P+50);local miny=sy
 for t=1,160 do H.wait(1,0x40);if H.r8(H.R+5)==2 then miny=math.min(miny,H.r16(H.P+50)) end end
 H.log('CLIFF startY='..sy..' minY='..miny);H.shot('cliff_on')
 H.check(sy>=592 and miny<592,'Mt Crenel crosses confirmed cliff Y=024F boundary')
end)
