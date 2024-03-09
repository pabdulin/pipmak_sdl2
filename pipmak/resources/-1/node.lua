slide { "prefs.png", cursor = pipmak.hand, mousemode = pipmak.joystick }

local okbtn = patch { x = 530, y = 441, w = 86, h = 30, image = "ok.png" }

handle {
	x = 533, y = 443, w = 78, h = 22,
	onmouseup = function()
		pipmak.gotonode(0)
	end,
	onhilite = function(self, h)
		if h then okbtn:setimage("ok_h.png")
		else okbtn:setimage("ok.png") end
	end
}

onkeydown (function(key)
	if key == pipmak.key_esc then
		pipmak.gotonode(0)
		return true
	else
		return false
	end
end)
