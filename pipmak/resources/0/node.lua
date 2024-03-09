--[[
This file shows how to programmatically make a row of buttons with identical behavior.
Coding each button separately may have led to a more readable file, but this way is
more elegant and easier to maintain.
]]

slide { "background.png", cursor = pipmak.hand, mousemode = pipmak.joystick }

local buttons = {"openproject", "newproject", "opensavedgame", "savegame", "continue", "preferences", "about", "quit"}
local enabled = {true, true, true, pipmak.cansavegame(), pipmak.cansavegame(), true, true, true}

--[[ not used here, but it may be useful later...
local function enable(i, e)
	enabled[i] = e
	pipmak.getpatch(i):setimage(buttons[i]..({[true]="", [false]="_i"})[e]..".png")
end
]]

local actions = {
	function()
		pipmak.dissolve()
		pipmak.openproject()
	end,
	function()
		pipmak_internal.newproject()
	end,
	function()
		pipmak.opensavedgame()
	end,
	function()
		pipmak.savegame()
	end,
	function()
		pipmak.dissolve()
		pipmak.thisnode():closeoverlay()
	end,
	function()
		pipmak.gotonode(-1)
	end,
	function()
		pipmak.gotonode(-2)
	end,
	function()
		pipmak.quit()
	end
}

hotspotmap "hotspotmap.png"

for i = 1, table.getn(buttons) do
	patch {
		x = 196, y = 144+(i-1)*40, w = 248, h = 48,
		image = buttons[i]..({[true]="", [false]="_i"})[enabled[i]]..".png"
	}
	hotspot {
		onmouseup = function(self)
			local i = self:getid()
			if enabled[i] then
				actions[i]()
			end
		end,
		onhilite = function(self, h)
			local i = self:getid()
			if enabled[i] then
				if h then
					pipmak.getpatch(i):setimage(buttons[i].."_h.png")
				else
					pipmak.getpatch(i):setimage(buttons[i]..".png")
				end
			end
		end
	}
end

onkeydown (function(key)
	if key == pipmak.key_esc then
		pipmak.dissolve()
		pipmak.thisnode():closeoverlay()
		return true
	else
		return false
	end
end)
