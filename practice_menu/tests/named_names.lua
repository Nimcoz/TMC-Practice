local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
local function stringAt(p)
 local s='';for i=0,80 do local c=H.r8(p+i);if c==0 then return s end;s=s..string.char(c) end;error('unterminated label')
end
local function row(y)
 local s='';for x=3,28 do local c=H.r16(0x02034CB0+2*(y*32+x))&0x3FF;s=s..string.char(c+32) end
 return s:gsub(' +$','')
end
H.run(function()
 H.menu(26,1);H.press(1)
 local total=0
 for _,k in ipairs({1,3,4,6,7,8,9}) do
  local count=H.r16(H.L.actorNameCounts+k*2);local table=H.r32(H.L.actorNames+k*4)
  for id=0,count-1 do
   H.option('actorRawKind',k);H.option('actorRawId',id);H.option('actorRawType',0);H.wait(2)
   local expected=stringAt(H.r32(table+id*4))
   if k==6 and id==0 then expected=stringAt(H.r32(H.L.groundItemNames)) end
   local a,b=row(13),row(14);local joined=a..(b~='' and ' '..b or '')
   H.check(joined==expected or a..b==expected,'name rendered '..k..'/'..id..' '..expected)
   total=total+1
  end
 end
 H.check(total==546,'all 546 native table slots have visible labels')
 for _,v in ipairs({{3,0,'OCTOROK'},{3,0x36,'MAZAAL HEAD'},{7,0x45,'ANJU'},
                   {6,5,'POT'},{7,5,'UNUSED ZELDA FOLLOWER'},{9,2,'VERTICAL MINISH PATH BACKGROUND'}}) do
  H.option('actorRawKind',v[1]);H.option('actorRawId',v[2]);H.wait(2)
  local a,b=row(13),row(14);H.check(a..(b~='' and ' '..b or '')==v[3],'independent expected name '..v[3])
  H.shot('name_'..v[1]..'_'..v[2])
 end
 H.option('actorRawKind',6);H.option('actorRawId',0);H.option('actorRawType',0x54);H.wait(2)
 H.check(row(13)=='GREEN RUPEE','ground-item TYPE is named');H.shot('ground_green_rupee')
 H.menu(29,2);H.press(0x10);H.check(row(13)=='BLUE RUPEE','real right input changes displayed item name')
 H.menu(29,9);H.press(1);H.check(row(6)=='BLUE RUPEE','confirmation identifies actual selected item');H.shot('named_confirmation')
 H.press(1);H.menu(29,1);H.option('actorRawId',0xFF);H.wait(2)
 H.check(row(13)=='NO NATIVE HANDLER','out-of-range raw ID remains explicit');H.close()
end)
