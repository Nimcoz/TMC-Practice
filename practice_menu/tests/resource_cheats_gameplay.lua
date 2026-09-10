local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local stats=0x2002AE8
H.run(function()
 H.check(H.r8(H.PM+H.L.resourceCheats)==0,'new resource cheats default OFF')
 local flags=emu:readRange(0x2002C9C,0x200);local inv=emu:readRange(0x2002B32,34)
 H.menu(16,4)
 for row=4,7 do H.press(1);if row<7 then H.press(0x80) end end
 H.check(H.r8(H.PM+H.L.resourceCheats)==15,'native menu navigation independently enables all four new cheats')
 H.shot('cheats_all_on');H.close()
 H.check(emu:readRange(0x2002B32,34)==inv and emu:readRange(0x2002C9C,0x200)==flags,'enabling cheats grants no items or progression flags')
 H.check(H.r8(stats+4)==0 and H.r8(stats+5)==0,'ammo cheats do nothing without bombs/bow ownership')
 H.check(H.r16(stats+24)==100,'infinite rupees respects original small wallet')
 H.action(14,0);H.warp(0,0,504,504);H.openFloor();local equipped=emu:readRange(stats+12,2)
 for upgrade=0,3 do
  H.log('FIXTURE owned capacity tier '..upgrade..' and depleted counters')
  emu:write8(stats,upgrade);emu:write8(stats+6,upgrade);emu:write8(stats+7,upgrade)
  emu:write8(stats+4,1);emu:write8(stats+5,1);emu:write16(stats+24,1);emu:write8(stats+2,8);emu:write8(H.P+69,8)
  H.wait(4)
  H.check(H.r8(stats+4)==({10,30,50,99})[upgrade+1] and H.r8(stats+5)==({30,50,70,99})[upgrade+1] and H.r16(stats+24)==({100,300,500,999})[upgrade+1],'native bomb/quiver/wallet capacities tier '..upgrade)
  H.check(H.r8(stats+2)==H.r8(stats+3) and H.r8(H.P+69)==H.r8(stats+3),'both health mirrors refill to existing maximum')
 end
 H.check(emu:readRange(stats+12,2)==equipped,'refill never changes equipped items')
 -- Native weapon inputs, not a counter-only demonstration. Give All owns
 -- remote bombs and light arrows; select them on A as a test fixture.
 local base=emu:saveStateBuffer()
 for _,case in ipairs({{8,4,2,'bombs',2},{10,5,4,'arrows',4}}) do
  local amount={};local spawned={}
  for on=0,1 do
   emu:loadStateBuffer(base);H.option('resourceCheats',on==1 and case[3] or 0);emu:write8(stats+12,case[1]);emu:write8(stats+case[2],5);H.wait(8)
   local before=H.r8(stats+case[2]);spawned[on]=false
   for frame=1,26 do
    H.wait(1,frame<=3 and 1 or 0)
    for i=0,71 do local e=0x30015A0+i*0x88;if H.r32(e+4)~=0 and H.r8(e+8)==8 and H.r8(e+9)==case[5] then spawned[on]=true end end
   end
   amount[on]=H.r8(stats+case[2])
   H.log(string.format('NATIVE_USE %s on=%d before=%d after=%d playeritem=%s',case[4],on,before,amount[on],tostring(spawned[on])))
   if on==0 then H.check(amount[on]<before,'OFF: native '..case[4]..' use consumes ammunition') end
   if on==1 then H.check(amount[on]==99,'ON: native '..case[4]..' use retains full ammunition') end
   H.check(spawned[on],'actual native '..case[4]..' entity created, cheat '..on)
   H.shot(case[4]..'_'..on)
  end
 end
 emu:loadStateBuffer(base);H.option('resourceCheats',15);H.action(17,1);H.wait(20)
 H.check(H.r8(stats+4)==0 and H.r8(stats+5)==0 and H.item(8)==0 and H.item(10)==0,'Delete All stays empty while ammo cheats remain ON')
 H.action(13,0);H.check(H.r8(H.PM+H.L.resourceCheats)==0,'Settings Reset disables all new cheats')
 emu:write16(stats+24,12);H.wait(10);H.check(H.r16(stats+24)==12,'OFF: wallet no longer refills')
 -- Real enemy contact, same native state with Hearts OFF/ON; Invincibility OFF.
 emu:loadStateBuffer(base);H.option('resourceCheats',0)
 emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20);H.log('FIXTURE temporary TABIDACHI enemy-list gate')
 H.warp(0,0,504,504);H.option('freezeEnemies',1);H.wait(60)
 local enemy
 for i=0,71 do local e=0x30015A0+i*0x88;if H.r32(e+4)~=0 and H.r8(e+8)==3 and H.r8(e+9)==0 then enemy=e;break end end
 assert(enemy,'native enemy missing')
 local combat=emu:saveStateBuffer()
 for on=0,1 do
  emu:loadStateBuffer(combat);H.option('resourceCheats',on);emu:write8(H.PM+16,0)
  emu:write32(H.P+44,H.r32(enemy+44));emu:write32(H.P+48,H.r32(enemy+48));local hp=H.r8(stats+2);local knocked=false
  for t=1,30 do H.wait(1);if H.r8(H.P+66)>0 then knocked=true end end
  H.check(knocked,'native contact knockback remains with Infinite Hearts '..on)
  H.check(on==0 and H.r8(stats+2)<hp or on==1 and H.r8(stats+2)==H.r8(stats+3),'contact health behaves correctly with Hearts '..on)
  H.shot('hearts_contact_'..on)
 end
 emu:loadStateBuffer(base);H.option('resourceCheats',15);H.warp(3,1,336,272)
 if H.r8(H.S+139)~=0 then
  emu:write16(stats+24,7);H.wait(5);H.check(H.r16(stats+24)==7,'resource refill suspends during actual native scripted control')
 end
end)
