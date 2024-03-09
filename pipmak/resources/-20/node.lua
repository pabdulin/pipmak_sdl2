panel { "dialogbackground.png", h = 282, mousemode = pipmak.joystick }

patch { x = 204, y = 17, image = "title.png" }
patch { x = 32, y = 53, image = "text.png" }

local btn1 = patch { x = 31, y = 178, image = "openmainlua.png" }
local btn2 = patch { x = 31, y = 206, image = "opennodefolder.png"}
local btn3 = patch { x = 31, y = 234, image = "opennodelua.png" }
local btn4 = patch { x = 381, y = 234, image = "../-1/ok.png" }

hotspotmap "hotspotmap.png"

hotspot {
	onhilite = function(self, h)
		btn1:setimage("openmainlua" .. (h and "_h" or "") .. ".png")
	end,
	onmouseup = function(self)
		pipmak_internal.openfile("main.lua")
	end
}

hotspot {
	onhilite = function(self, h)
		btn2:setimage("opennodefolder" .. (h and "_h" or "") .. ".png")
	end,
	onmouseup = function(self)
		pipmak_internal.openfile("1")
	end
}

hotspot {
	onhilite = function(self, h)
		btn3:setimage("opennodelua" .. (h and "_h" or "") .. ".png")
	end,
	onmouseup = function(self)
		pipmak_internal.openfile("1/node.lua")
	end
}

hotspot {
	onhilite = function(self, h)
		btn4:setimage("../-1/ok" .. (h and "_h" or "") .. ".png")
	end,
	onmouseup = function(self)
		pipmak.thisnode():closeoverlay()
	end
}
