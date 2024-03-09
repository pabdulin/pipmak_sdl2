local gui = pipmak.dofile("../resources/gui/gui.lua")


local width = pipmak.screensize() - 20

gui.windoid("Lua Command Line", width, 59, { rely = 1.0, absy = -77 })

-- help box
local helpbox = patch { x = width-8, y = 8, image = "help.png" }
local helpnode
handle {
	x = width-8, y = 8, w = 14, h = 14,
	onhilite = function(self, h) helpbox:setimage(h and "help_h.png" or "help.png") end,
	onmouseup = function()
		if helpnode and helpnode.closeoverlay then
			helpnode:closeoverlay()
			helpnode = nil
		else
			helpnode = pipmak.overlaynode(-13)
		end
	end
}

-- forward declarations
local historylength, hindex, historypatch, histhighlight, historyoffset, displayedhistorylength
local drawoutputimg, drawhistoryimg, showhistory, scrollup, scrolldown

-- text input field
local te
te = texteditor {
	x = 15 + 3, y = 44 + 2, w = width-14-17 - 5,
	maxlength = 300,
	onkeydown = function(key, shift)
		if key == pipmak.key_return then
			if historypatch:isvisible() then
				te:paste(commandlinehistory[3*hindex + (shift and 3 or 1)])
				showhistory(false)
			else
				local line = te:text()
				if line ~= "" then
					local result = ""
					-- try compiling it with a "return" in front first, in case it's an expression (function calls can be both expressions and statements, and we want to treat them as expressions)
					local chunk, err = loadstring("return " .. line)
					if not chunk then
						chunk, err = loadstring(line)
					end
					if chunk then
						local savednode = pipmak.thisnode()
						pipmak_internal.pretendnode(pipmak.backgroundnode())
						local ret = { pcall(chunk) }
						pipmak_internal.pretendnode(savednode)
						if ret[1] then
							_ = ret[2]
							if table.getn(ret) >= 2 then
								result = tostring(_)
								for i = 3, table.getn(ret) do result = result .. ", " .. tostring(ret[i]) end
							else
								result = "(nil)"
							end
						else
							err = ret[2]
						end
					end
					if err then
						local i = string.find(err, "\"]:1: ", 1, true)
						if i ~= nil then
							err = string.sub(err, i + 6)
						end
					end
					commandlinehistory[3*historylength + 1] = line
					commandlinehistory[3*historylength + 2] = not err
					commandlinehistory[3*historylength + 3] = err or result
					historylength = historylength + 1
					drawoutputimg()
				end
				te:selection(0, -1)
			end
		elseif key == pipmak.key_arrow_up and not shift then
			if historypatch:isvisible() then
				if (hindex > 0) then
					hindex = hindex - 1
					if (hindex < historyoffset) then scrollup() end
					histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
				end
			elseif historylength > 0 then
				showhistory(true)
			end
		elseif key == pipmak.key_arrow_down and not shift then
			if historypatch:isvisible() then
				if (hindex < historylength-1) then
					hindex = hindex + 1
					if (hindex > historyoffset + displayedhistorylength - 1) then scrolldown() end
					histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
				else
					showhistory(false)
				end
			end
		else
			return false
		end
		return true
	end
}
patch { x = 15 - 3, y = 44 - 3, w = width-14 + 6, leftmargin = 4.5, rightmargin = 4.5, image = "../resources/gui/textfield.png" }
handle { x = 15, y = 44, w = width-14-17, h = 17, cursor = pipmak.ibeam, onmousedown = function() te:mouse(true) end, onmousestilldown = function() te:mouse(false) end }

-- history popup triangle
patch { x = width - 11, y = 47, image = "../resources/gui/popuptriangle.png" }
handle {
	x = width - 16, y = 44, w = 17, h = 17,
	onmousedown = function()
		if historylength > 0 then
			showhistory(not historypatch:isvisible())
		end
		te:focus()
	end
}

-- text output field
local outputimg = pipmak.newimage(width-34, 14)
patch { x = 18, y = 25, image = outputimg }

-- table inspector button
local inspectorp = patch { x = width - 13, y = 26, image = "../resources/gui/crosshair.png" }
local inspectorh = handle {
	x = width - 15, y = 24, w = 17, h = 17,
	onhilite = function(self, hi)
		inspectorp:setimage(hi and inspectorp:isvisible() and "../resources/gui/crosshair_h.png" or "../resources/gui/crosshair.png")
	end,
	onmouseup = function()
		if inspectorp:isvisible() then
			if type(_) == "table" then
				pipmak.overlaynode(-12):message("goto", _)
			else
				-- someone must have assigned a different value to _ in the meantime
				inspectorp:setvisible(false)
			end
		end
	end
}

-- history
local historymousex, historymousey
local function historyonmousemove(self)
	local mx, my = pipmak.mouseloc()
	if mx ~= historymousex or my ~= historymousey then
		historymousex, historymousey = mx, my
		local x, y = self:location()
		local i = math.floor((my - y - 1)/16)
		if i < 0 then i = 0
		elseif i >= displayedhistorylength then i = displayedhistorylength - 1 end
		if hindex ~= i + historyoffset then
			hindex = i + historyoffset
			histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
		end
	end
end
local historyimg = pipmak.newimage(width-10, 10*16+3)
historypatch = patch { x = 13, y = 39-10*16, image = historyimg, visible = false }
local historyhandle = handle {
	x = 0, y = 0, w = 1, h = 1,
	enabled = false,
	onmousewithin = historyonmousemove,
	onmousestilldown = historyonmousemove,
	onmouseup = function(self)
		te:paste(commandlinehistory[3*hindex + (shift and 3 or 1)])
		showhistory(false)
		te:focus()
	end
}

histhighlight = patch { x = 0, y = 0, visible = false, image = pipmak.newimage(width-12, 17):color(0, .1, .8, .4):fill():color(0, .1, .8, .1):fill(1, 1, width-14, 15) }

local downarrowp = patch { x = 8+width/2-14, y = 41, visible = false, image = "downarrow.png" }
local uparrowp = patch { x = 0, y = 0, visible = false, image = "uparrow.png" }
local downarrowh = handle {
	x = 8+width/2-14, y = 41,
	w = 28, h = 10,
	enabled = false,
	onmousedown = function()
		scrolldown()
		pipmak.schedule(0.5,
			function()
				if not pipmak.mouseisdown() then return nil end
				scrolldown()
				return 0.1
			end
		)
		te:focus()
	end
}
local uparrowh = handle {
	x = 0, y = 0,
	w = 28, h = 10,
	enabled = false,
	onmousedown = function()
		scrollup()
		pipmak.schedule(0.5,
			function()
				if not pipmak.mouseisdown() then return nil end
				scrollup()
				return 0.1
			end
		)
		te:focus()
	end
}

-- initialization
onenternode(
	function()
		if commandlinehistory == nil then commandlinehistory = {} end
		historylength = table.getn(commandlinehistory)/3
		drawoutputimg()
	end
)

-- commonly used code

function drawoutputimg()
	outputimg:color(0, 0, 0, 0):fill()
	if historylength > 0 then
		outputimg:color(0, 0, 0):drawtext(0, 0, commandlinehistory[3*historylength-2])
		if commandlinehistory[3*historylength-1] then
			outputimg:color(0, .3, 0)
		else
			outputimg:color(.5, 0, 0)
		end
		outputimg:drawtext(width-34, 0, commandlinehistory[3*historylength], pipmak.right)
	end
	inspectorp:setvisible(type(_) == "table")
end

function drawhistoryimg()
	historyimg:color(1, 1, 1, .9):fill()
	historyimg:color(0, 0, 0, 0):fill(0, 0, width-10, (10-displayedhistorylength)*16)
	local y, idx
	for i = 0, 9 do
		y = 2 + i*16
		idx = 3*(i+historyoffset-10+displayedhistorylength)
		if idx >= 0 then
			historyimg:color(0, 0, 0):drawtext(5, y, commandlinehistory[idx+1])
			if commandlinehistory[idx+2] then
				historyimg:color(0, .3, 0)
			else
				historyimg:color(.5, 0, 0)
			end
			historyimg:drawtext(width-29, y, commandlinehistory[idx+3], pipmak.right)
		end
	end
end

local function enablescrollbuttons()
	uparrowp:setvisible(historyoffset ~= 0)
	downarrowp:setvisible(historyoffset ~= historylength - displayedhistorylength)
	uparrowh:enable(historyoffset ~= 0)
	downarrowh:enable(historyoffset ~= historylength - displayedhistorylength)
end

function scrollup()
	if historyoffset > 0 then
		historyoffset = historyoffset - 1
		if hindex > historyoffset + displayedhistorylength - 1 then
			hindex = historyoffset + displayedhistorylength - 1
		end
		drawhistoryimg()
		enablescrollbuttons()
		histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
	end
end

function scrolldown()
	if historyoffset < historylength - displayedhistorylength then
		historyoffset = historyoffset + 1
		if hindex < historyoffset then
			hindex = historyoffset
		end
		drawhistoryimg()
		enablescrollbuttons()
		histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
	end
end

function showhistory(v)
	historypatch:setvisible(v)
	histhighlight:setvisible(v)
	historyhandle:enable(v)
	if v then
		hindex = historylength-1 --zero-based index
		displayedhistorylength = historylength < 10 and historylength or 10
		historyoffset = historylength - displayedhistorylength --zero-based index of the first visible row
		drawhistoryimg()
		histhighlight:moveto(14, 24 + 16*(-displayedhistorylength + hindex+1 - historyoffset))
		enablescrollbuttons()
		uparrowh:moveto(8+width/2-14, 30-displayedhistorylength*16)
		uparrowp:moveto(8+width/2-14, 30-displayedhistorylength*16)
		historyhandle:move { x = 13, y = 39-displayedhistorylength*16, w = width-10, h = displayedhistorylength*16+3 }
	else
		uparrowp:setvisible(false)
		downarrowp:setvisible(false)
		downarrowh:enable(false)
		--moving handles inside the panel while they're disabled makes mouse insideness computation faster
		uparrowh:moveto(0, 0)
		historyhandle:move { x = 0, y = 0, w = 1, h = 1 }
	end
end
