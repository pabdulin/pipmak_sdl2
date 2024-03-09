slide { "about.png", cursor = pipmak.hand, mousemode = pipmak.joystick }

local vers = string.format("%03d", 100*pipmak.version())

patch { x = 260, y = 105, image = pipmak.newimage(120, 18):drawtext(60, 0, "Version "..string.sub(vers, 1, 1).."."..string.sub(vers, 2, 2).."."..string.sub(vers, 3, 3), "../resources/VeraBd.ttf", 13, pipmak.center) }

patch { x = 120, y = 210, image = pipmak.newimage(400, 18):drawtext(200, 0, "© 2004–2008 Christian Walther ‹cwalther@gmx.ch›", "../resources/Vera.ttf", 13, pipmak.center) }

handle {
	x = 0, y = 0, w = 640, h = 480,
	onmouseup = function()
		pipmak.gotonode(0)
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
