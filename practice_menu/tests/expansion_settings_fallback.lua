local out=assert(os.getenv('TMC_TEST_OUTPUT'))
local H=assert(loadfile(out..'../../tests/gameplay_harness.lua'))()
H.run(function()
 local badBoth=out:find('bad_both')~=nil or out:find('bad_crc')~=nil
 H.check(H.r8(H.PM+H.L.menuTheme)==(badBoth and 0 or 2),badBoth and 'both invalid records fall back to defaults' or 'bad primary falls back to valid mirror from older23/21-page schema')
 H.menu(13,4);H.shot('settings_fallback');H.close()
 H.check(H.r8(H.M+4)==2,'gameplay remains usable with corrupted settings')
end)
