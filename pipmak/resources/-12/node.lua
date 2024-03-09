local gui = pipmak.dofile("../resources/gui/gui.lua")


local width = 300
local numlines = 10

gui.windoid("Lua Table Inspector", width, numlines*16+42)


local history = {}

-- forward declarations
local goto, update, updateScrolling, mykeytostring, mykeytotitle, myvaluetostring
local scrollbar

local backButtonP = patch { x = 15, y = 25, image = "../resources/gui/triangle_left.png" }
backButtonP:setalpha(0.3)
local backButtonH
backButtonH = handle {
	x = 16, y = 26, w = 11, h = 13,
	enabled = false,
	onhilite = function(self, hi)
		backButtonP:setimage(hi and "../resources/gui/triangle_left_h.png" or "../resources/gui/triangle_left.png")
	end,
	onmouseup = function()
		table.remove(history)
		table.remove(history)
		local n = table.getn(history)
		assert(n >= 2)
		update(history[n-1], history[n])
		if n == 2 then
			backButtonP:setalpha(0.3)
			backButtonH:enable(false)
		end
	end
}

local titleImg = pipmak.newimage(width-16-13, 14)

local w1 = math.floor((width - 16 - 8 - 12)/3) -- width - borders - column spacing - scrollbar width
local tableimg = pipmak.newimage(width - 16 - 12, numlines*16)

local count = 0
local scroll = 0
local keys, values

patch { x = 29, y = 25, image = titleImg }
patch { x = 16, y = 42, image = tableimg }
local crosshairpatches = {}
for l = 0, numlines-1 do
	local i = l --need a copy
	local p = patch { x = width - 16 - 12, y = 44 + l*16, image = "../resources/gui/crosshair.png", visible = false }
	handle {
		x = width - 16 - 12, y = 44 + l*16,
		w = 13, h = 13,
		onhilite = function(self, hi)
			p:setimage(hi and p:isvisible() and "../resources/gui/crosshair_h.png" or "../resources/gui/crosshair.png")
		end,
		onmouseup = function()
			if p:isvisible() then goto(values[i+scroll+1], mykeytotitle(keys[i+scroll+1])) end
		end
	}
	crosshairpatches[l] = p
end

function mykeytostring(v)
	if type(v) == "string" then
		local s, e = string.find(v, "[_%a][_%w]*")
		if s == 1 and e == string.len(v) then return v
		else return string.format("%q", v) end
	else
		return tostring(v)
	end
end

function mykeytotitle(v)
	if type(v) == "string" then
		local s, e = string.find(v, "[_%a][_%w]*")
		if s == 1 and e == string.len(v) then return v
		else return string.format("[%q]", v) end
	else
		return "[" .. tostring(v) .. "]"
	end
end

function myvaluetostring(v)
	if type(v) == "string" then return string.format("%q", v)
	else return tostring(v) end
end

function goto(thetable, name)
	table.insert(history, thetable)
	table.insert(history, name)
	update(thetable, name)
	if table.getn(history) > 2 then
		backButtonP:setalpha(1)
		backButtonH:enable(true)
	else
		backButtonP:setalpha(0.3)
		backButtonH:enable(false)
	end
end

function update(thetable, name)
	titleImg:color(0, 0, 0, 0):fill()
	local title = tostring(thetable)
	if name ~= nil then title = name .. " = " .. title end
	titleImg:color(0, 0, 0, 1):drawtext(1, 0, title)
	count = 0
	scroll = 0
	keys, values = {}, {}
	for k, v in pairs(thetable) do
		count = count + 1
		keys[count] = k
	end
	table.sort(keys, function(x, y) local tx, ty = type(x), type(y) if tx == ty then return x < y else return tx < ty end end)
	for i, k in pairs(keys) do
		values[i] = thetable[k]
	end
	scrollbar:setrange(count)
	scrollbar:setvalue(scroll)
	updateScrolling()
end

function updateScrolling()
	for i = 0, numlines-1 do
		local rowno = i + scroll
		local r, g, b
		if math.mod(rowno, 2) == 0 then
			r, g, b = 1, 1, 1
		else
			r, g, b = 0.97, 0.96, 0.92
		end
		tableimg:color(r, g, b)
		if rowno < count then
			tableimg:fill(0, i*16, w1, 16)
			tableimg:color(0, 0, 0):drawtext(4, 1 + i*16, mykeytostring(keys[rowno+1]))
			tableimg:color(r, g, b):fill(w1, i*16, width, 16)
			tableimg:color(0, 0, 0):drawtext(w1 + 8, 1 + i*16, myvaluetostring(values[rowno+1]))
		else
			tableimg:fill(0, i*16, width, 16)
		end
		crosshairpatches[i]:setvisible(rowno < count and type(values[rowno+1]) == "table")
	end
end

scrollbar = gui.scrollbar {
	x = width - 12, y = 42, h = numlines*16,
	range = count, window = numlines, value = 0,
	onscroll = function(v)
		scroll = v
		updateScrolling()
	end
}

messages {
	goto = goto
}
