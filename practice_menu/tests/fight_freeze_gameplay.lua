local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 H.action(14,0);emu:write8(0x2002C9E,H.r8(0x2002C9E)|0x20);H.log('FIXTURE TABIDACHI enemy list gate')
 emu:write8(H.PM+16,1);H.option('freezeEnemies',1)
 local sounds={};emu:setBreakpoint(function() local s=emu:readRegister('r0')&0xFFFF;sounds[s]=(sounds[s] or 0)+1 end,0x80A3268)
 H.warp(0x70,5,192,380);H.wait(12,0x80);H.wait(220);H.shot('fight_spawn')
 H.log(string.format('PLAYER area=%02X/%02X xy=%d,%d action=%d control=%d origin=%d,%d',H.r8(H.R+4),H.r8(H.R+5),H.r16(H.P+46),H.r16(H.P+50),H.r8(H.P+12),H.r8(H.S+139),H.r16(H.R+6),H.r16(H.R+8)))
 local head=0x3003DB0;local e=H.r32(head+4)
 for i=1,32 do if e==head or e==0 then break end
  H.log(string.format('MANAGER %08X kind=%d id=%02X type=%d action=%d start=%04X end=%04X',e,H.r8(e+8),H.r8(e+9),H.r8(e+10),H.r8(e+12),H.r16(e+60),H.r16(e+62)));e=H.r32(e+4)
 end
 local function enemies()
  local found={}
  for i=0,71 do local e=0x30015A0+i*0x88
   if H.r32(e+4)~=0 and H.r8(e+8)==3 and H.r8(e+69)~=0 then found[#found+1]=e end
  end
  return found
 end
 local targets=enemies();H.log('native fight enemies='..#targets)
 for _,e in ipairs(targets) do H.log(string.format('ENEMY %08X id=%02X hp=%d coll=%02X draw=%d monitored=%02X',e,H.r8(e+9),H.r8(e+69),H.r8(e+16),H.r8(e+24)&3,H.r8(e+109))) end
 H.check(#targets>=2,'native multi-enemy fight spawned with Freeze already ON')
 local wizz,visible=0,true
 for _,e in ipairs(targets) do if H.r8(e+9)>=0x27 and H.r8(e+9)<=0x29 then wizz=wizz+1;if H.r8(e+16)&0x80==0 or H.r8(e+24)&3==0 then visible=false end end end
 H.check(wizz>0 and visible,'native Wizzrobes finish appearing and become collidable while Freeze ON')
 local roomFlagsBefore=emu:readRange(0x2034364,0x34)
 -- Native fight manager stores completion as local flag 3A, not a room flag.
 local flagIndex=H.r16(0x2033A94)+0x3A
 local flagAddress=0x2002C9C+math.floor(flagIndex/8);local flagMask=1<<(flagIndex%8)
 local completedBefore=H.r8(flagAddress)&flagMask
 local deaths=0;local projectileDamage=false;local selected=0
 local drops={}
 local function trackDrops()
  for i=0,71 do local e=0x30015A0+i*0x88
   if H.r32(e+4)~=0 and H.r8(e+8)==6 and H.r8(e+9)==0 then
    local key=string.format('%08X/%02X',e,H.r8(e+10))
    if not drops[key] then H.log('NATIVE_GROUND_ITEM '..key);drops[key]=true end
   end
  end
 end
 for _,e in ipairs(targets) do
  local id=H.r8(e+9);local hp=H.r8(e+69);selected=selected+1
  local weapon=selected==1 and 10 or 6
  emu:write8(0x2002AF4,weapon);H.log('FIXTURE equip owned '..(weapon==10 and 'Light Arrow' or 'Four Sword'))
  for attempt=1,20 do
   if H.r32(e+4)==0 or H.r8(e+8)~=3 or H.r8(e+69)==0 then break end
   local positions={{0,24,0},{0,-24,4},{24,0,6},{-24,0,2}};local d=positions[(attempt-1)%4+1]
   emu:write32(H.P+44,H.r32(e+44)+d[1]*65536);emu:write32(H.P+48,H.r32(e+48)+d[2]*65536);emu:write8(H.P+20,d[3]);H.wait(5)
   H.wait(6,1);H.wait(45)
   trackDrops()
   if weapon==10 and H.r8(e+69)<hp then projectileDamage=true end
   H.log(string.format('HIT id=%02X weapon=%d attempt=%d hp=%d action=%d kind=%d',id,weapon,attempt,H.r8(e+69),H.r8(e+12),H.r8(e+8)))
  end
  if H.r8(e+69)==0 or H.r32(e+4)==0 or H.r8(e+8)~=3 then deaths=deaths+1 end
 end
 H.wait(240);H.shot('fight_completed')
 trackDrops();H.check(next(drops)~=nil,'native enemy deaths produce collectible Ground Item drops')
 H.check(projectileDamage,'native player projectile damages a frozen enemy')
 H.check(#targets>0 and deaths==#targets and #enemies()==0,'all native fight enemies die and are removed while Freeze remains ON')
 local roomFlagsAfter=emu:readRange(0x2034364,0x34)
 for i=1,#roomFlagsBefore do if roomFlagsBefore:byte(i)~=roomFlagsAfter:byte(i) then H.log(string.format('NATIVE_ROOM_FLAG_BYTE %02X %02X -> %02X',i-1,roomFlagsBefore:byte(i),roomFlagsAfter:byte(i))) end end
 H.log(string.format('NATIVE_FIGHT_COMPLETION local=003A absolute=%03X before=%d after=%d',flagIndex,completedBefore,H.r8(flagAddress)&flagMask))
 H.check(completedBefore==0 and H.r8(flagAddress)&flagMask~=0,'native fight manager sets its actual local completion flag without direct flag writes')
 local count=0;for sound,times in pairs(sounds) do if sound>99 then count=count+times;H.log(string.format('NATIVE_SFX %04X count=%d',sound,times)) end end
 H.check(count>0,'native sound requests continue through frozen-enemy combat (not an audible listening test)')
end)
