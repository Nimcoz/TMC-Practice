local frame = 0
local phase = "boot"
local phase_frame = 0
local gameplay_frames = 0
local last_task = -1
local task_frames = 0
local out_dir = "C:/Users/user/Documents/Codex/2026-08-30/files-mentioned-by-the-user-tmc/practice_menu/test_results/dialog_restore/"
local log_path = out_dir .. "dialog_restore.log"
local done_path = out_dir .. "dialog_restore.done"

local MAIN = 0x03001000
local PRACTICE = 0x0203D000
local MESSAGE = 0x02000050
local MESSAGE_SCRATCH = 0x02000040
local TEXT_GFX = 0x02000D00
local TEXT_RENDER = 0x02022780
local MESSAGE_CHOICES = 0x02024030

local KEY_A = 0x001
local KEY_SELECT = 0x004
local KEY_START = 0x008
local KEY_R = 0x100
local KEY_L = 0x200
local HOTKEY = KEY_L + KEY_R + KEY_SELECT

local old_message = 0
local old_render_message = 0
local old_scratch = 0
local old_choice = 0
local old_gfx = 0

local function log(tag)
    local f = io.open(log_path, "a")
    if not f then return end
    f:write(string.format(
        "%s frame=%d phase=%s task=%02X state=%02X sub=%02X menu=%02X page=%02X message=%02X renderMessage=%02X scratch=%02X choice=%02X gfx=%02X\n",
        tag, frame, phase, emu:read8(MAIN + 2), emu:read8(MAIN + 3), emu:read8(MAIN + 4),
        emu:read8(PRACTICE + 13), emu:read8(PRACTICE + 23), emu:read8(MESSAGE),
        emu:read8(TEXT_RENDER), emu:read8(MESSAGE_SCRATCH), emu:read8(MESSAGE_CHOICES),
        emu:read8(TEXT_GFX)))
    f:close()
end

local function set_phase(next_phase)
    phase = next_phase
    phase_frame = frame
    log("PHASE")
end

local function restore_originals()
    emu:write8(MESSAGE, old_message)
    emu:write8(TEXT_RENDER, old_render_message)
    emu:write8(MESSAGE_SCRATCH, old_scratch)
    emu:write8(MESSAGE_CHOICES, old_choice)
    emu:write8(TEXT_GFX, old_gfx)
end

local function finish(result)
    restore_originals()
    log(result)
    local f = io.open(done_path, "w")
    if f then f:write(result .. "\n"); f:close() end
    phase = "done"
    emu:setKeys(0)
end

callbacks:add("frame", function()
    frame = frame + 1
    if frame == 1 then os.remove(log_path); os.remove(done_path); log("SCRIPT_START") end

    local task = emu:read8(MAIN + 2)
    local state = emu:read8(MAIN + 3)
    local substate = emu:read8(MAIN + 4)
    local menu_open = emu:read8(PRACTICE + 13)
    if task ~= last_task then last_task = task; task_frames = 0 else task_frames = task_frames + 1 end
    local keys = 0

    if phase == "boot" then
        if task == 0 and frame >= 650 and ((frame - 650) % 120) < 4 then keys = KEY_START end
        if task == 1 then
            if state == 1 and task_frames >= 120 then emu:write8(MAIN + 3, 2) end
            if task_frames >= 90 and ((task_frames - 90) % 120) < 4 then keys = KEY_A end
        end
        if task == 2 and state == 2 and substate == 2 and emu:read32(PRACTICE) == 0x504D4331 then
            gameplay_frames = gameplay_frames + 1
            if gameplay_frames >= 45 then set_phase("warm_open") end
        end
        if frame > 2400 then finish("FAIL_BOOT_TIMEOUT") end
    elseif phase == "warm_open" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("warm_wait") end
    elseif phase == "warm_wait" then
        if menu_open == 1 then set_phase("warm_close")
        elseif frame - phase_frame > 180 then finish("FAIL_WARM_OPEN") end
    elseif phase == "warm_close" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("warm_close_wait") end
    elseif phase == "warm_close_wait" then
        if menu_open == 0 and task == 2 and state == 2 and substate == 2 then set_phase("inject_settle")
        elseif frame - phase_frame > 180 then finish("FAIL_WARM_CLOSE") end
    elseif phase == "inject_settle" then
        if frame - phase_frame >= 30 then
            old_message = emu:read8(MESSAGE)
            old_render_message = emu:read8(TEXT_RENDER)
            old_scratch = emu:read8(MESSAGE_SCRATCH)
            old_choice = emu:read8(MESSAGE_CHOICES)
            old_gfx = emu:read8(TEXT_GFX)
            emu:write8(MESSAGE, 3)
            emu:write8(TEXT_RENDER, 3)
            emu:write8(MESSAGE_SCRATCH, 0x53)
            emu:write8(MESSAGE_CHOICES, 0x52)
            emu:write8(TEXT_GFX, 0x56)
            log("DIALOG_INJECTED")
            set_phase("dialog_open")
        end
    elseif phase == "dialog_open" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("dialog_wait_menu") end
    elseif phase == "dialog_wait_menu" then
        if menu_open == 1 then
            if emu:read8(MESSAGE) ~= 0 then finish("FAIL_NATIVE_MESSAGE_NOT_CLEARED")
            else log("NATIVE_MENU_CLEARED_DIALOG"); set_phase("dialog_close") end
        elseif frame - phase_frame > 180 then finish("FAIL_DIALOG_OPEN") end
    elseif phase == "dialog_close" then
        if frame - phase_frame < 4 then keys = HOTKEY else set_phase("dialog_wait_restore") end
    elseif phase == "dialog_wait_restore" then
        if menu_open == 0 and task == 2 and state == 2 and substate == 2 then
            log("RESTORE_CHECK")
            if emu:read8(MESSAGE) ~= 3 or emu:read8(TEXT_RENDER) ~= 3 or
               emu:read8(MESSAGE_SCRATCH) ~= 0x53 or emu:read8(MESSAGE_CHOICES) ~= 0x52 or
               emu:read8(TEXT_GFX) ~= 0x56 then
                finish("FAIL_DIALOG_STATE_NOT_RESTORED")
            else
                finish("PASS")
            end
        elseif frame - phase_frame > 240 then finish("FAIL_DIALOG_CLOSE") end
    end

    emu:setKeys(keys)
end)
