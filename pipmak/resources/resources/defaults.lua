--constants:

pipmak.cubic = 0
pipmak.slide = 1
pipmak.equirect = 2
pipmak.panel = 3

pipmak.flushimagecache() --to *create* the image cache in the first place
pipmak.hand = pipmak.loadcursor("resources/cursor_hand.png", 7, 1)
pipmak.hand_open = pipmak.loadcursor("resources/cursor_hand_open.png", 8, 5)
pipmak.hand_closed = pipmak.loadcursor("resources/cursor_hand_closed.png", 6, 2)
pipmak.hand_left = pipmak.loadcursor("resources/cursor_hand_left.png", 2, 3)
pipmak.hand_right = pipmak.loadcursor("resources/cursor_hand_right.png", 15, 3)
pipmak.hand_lleft = pipmak.loadcursor("resources/cursor_hand_lleft.png", 2, 4)
pipmak.hand_rright = pipmak.loadcursor("resources/cursor_hand_rright.png", 14, 4)
pipmak.hand_up = pipmak.loadcursor("resources/cursor_hand_up.png", 8, 1)
pipmak.hand_down = pipmak.loadcursor("resources/cursor_hand_down.png", 4, 14)
pipmak.hand_forward = pipmak.loadcursor("resources/cursor_hand_forward.png", 5, 1)
pipmak.hand_back = pipmak.loadcursor("resources/cursor_hand_back.png", 8, 11)
pipmak.hand_lout = pipmak.loadcursor("resources/cursor_hand_lout.png", 2, 3)
pipmak.hand_rout = pipmak.loadcursor("resources/cursor_hand_rout.png", 14, 3)
pipmak.hand_zoom = pipmak.loadcursor("resources/cursor_hand_zoom.png", 14, 6)
pipmak.hand_zoomin = pipmak.loadcursor("resources/cursor_hand_zoomin.png", 14, 6)
pipmak.hand_zoomout = pipmak.loadcursor("resources/cursor_hand_zoomout.png", 14, 6)
pipmak.zip = pipmak.loadcursor("resources/cursor_zip.png", 8, 7)
pipmak.arrow = pipmak.loadcursor("resources/cursor_arrow.png", 2, 1)
pipmak.pencil = pipmak.loadcursor("resources/cursor_pencil.png", 2, 14)
pipmak.eyedropper = pipmak.loadcursor("resources/cursor_eyedropper.png", 2, 15)
pipmak.pan = pipmak.loadcursor("resources/cursor_pan.png", 9, 8)
pipmak.dot = pipmak.loadcursor("resources/cursor_dot.png", 4, 3)
pipmak.triangle = pipmak.loadcursor("resources/cursor_triangle.png", 9, 9)
pipmak.ibeam = pipmak.loadcursor("resources/cursor_ibeam.png", 5, 8)
pipmak_internal.specialcursor(0, pipmak.hand)
pipmak_internal.specialcursor(1, pipmak.triangle)
pipmak_internal.specialcursor(2, pipmak.dot)
pipmak_internal.specialcursor(3, pipmak.pan)
pipmak_internal.specialcursor(4, pipmak.eyedropper)

pipmak.joystick = 0
pipmak.direct = 1

pipmak.left = 0
pipmak.right = 1
pipmak.up = 2
pipmak.down = 3
pipmak.center = 4

pipmak.hotspot = 0
pipmak.handle = 1

pipmak.key_backspace = 8
pipmak.key_tab = 9
pipmak.key_clear = 12
pipmak.key_return = 13
pipmak.key_pause = 19
pipmak.key_esc = 27
pipmak.key_delete = 127
pipmak.key_kp_0 = 256
pipmak.key_kp_1 = 257
pipmak.key_kp_2 = 258
pipmak.key_kp_3 = 259
pipmak.key_kp_4 = 260
pipmak.key_kp_5 = 261
pipmak.key_kp_6 = 262
pipmak.key_kp_7 = 263
pipmak.key_kp_8 = 264
pipmak.key_kp_9 = 265
pipmak.key_kp_period = 266
pipmak.key_kp_divide = 267
pipmak.key_kp_multiply = 268
pipmak.key_kp_minus = 269
pipmak.key_kp_plus = 270
pipmak.key_kp_enter = 271
pipmak.key_kp_equals = 272
pipmak.key_arrow_up = 273
pipmak.key_arrow_down = 274
pipmak.key_arrow_right = 275
pipmak.key_arrow_left = 276
pipmak.key_insert = 277
pipmak.key_home = 278
pipmak.key_end = 279
pipmak.key_pageup = 280
pipmak.key_pagedown = 281
pipmak.key_f1 = 282
pipmak.key_f2 = 283
pipmak.key_f3 = 284
pipmak.key_f4 = 285
pipmak.key_f5 = 286
pipmak.key_f6 = 287
pipmak.key_f7 = 288
pipmak.key_f8 = 289
pipmak.key_f9 = 290
pipmak.key_f10 = 291
pipmak.key_f11 = 292
pipmak.key_f12 = 293
pipmak.key_f13 = 294
pipmak.key_f14 = 295
pipmak.key_f15 = 296

--pipmak functions

function pipmak.getpatch(i)
	return pipmak.thisnode().patches[i]
end

function pipmak.gethotspot(i)
	return pipmak.thisnode().controls[i]
end

function pipmak.gethandle(i)
	return pipmak.thisnode().controls[255+i]
end

function pipmak.registerprefsnode(node)
	if type(node) == "number" or type(node) == "string" then
		pipmak_internal.project.prefsnode = tostring(node)
	else
		error("bad argument #1 to `registerprefsnode' (string expected, got " .. type(node) .. ")", 2)
	end
end

function pipmak.gotoprefsnode()
	pipmak.overlaynode(pipmak_internal.project.prefsnode, true)
end

function pipmak.returnfromprefs()
	pipmak.overlaynode(pipmak_internal.project.prefsnode):closeoverlay()
end

function pipmak.cansavegame()
	return pipmak.backgroundnode() ~= nil and pipmak.backgroundnode():getname() ~= "0" and string.sub(pipmak.backgroundnode():getname(), 1, 1) ~= "-"
end

pipmak.loadimage = pipmak.getimage -- for backwards compatibility


--serialization functions

function serialize(d)
	local function do_serialize(d, tables)
		local f = ({
			["nil"] = function()
				return "x"
			end,
			["number"] = function(d)
				return "n"..string.pack(">n", d)
			end,
			["string"] = function(d)
				return "s"..string.pack(">a", d)
			end,
			["boolean"] = function(d)
				if d then return "r"
				else return "f" end
			end,
			["table"] = function(d, tables)
				if tables[d] == nil then
					tables[d] = tables.n
					tables.n = tables.n + 1
					local r = "t"
					for k, v in pairs(d) do
						r = r..do_serialize(k, tables)..do_serialize(v, tables)
					end
					r = r.."x"
					return r
				else
					return "b"..string.pack(">n", tables[d])
				end
		end
		})[type(d)]
		if f == nil then
			error("Can't serialize a " .. type(d) .. ".", 2)
		else
			return f(d, tables)
		end
	end
	return do_serialize(d, {n = 0})
end

function deserialize(s)
	local function do_deserialize(s, i, tables)
		local f = ({
			["x"] = function(s, i)
				return i, nil
			end,
			["n"] = function(s, i)
				return string.unpack(s, ">n", i)
			end,
			["s"] = function(s, i)
				return string.unpack(s, ">a", i)
			end,
			["r"] = function(s, i)
				return i, true
			end,
			["f"] = function(s, i)
				return i, false
			end,
			["t"] = function(s, i, tables)
				local r = {}
				tables[tables.n] = r
				tables.n = tables.n + 1
				local k, v
				while string.sub(s, i, i) ~= "x" do
					i, k = do_deserialize(s, i, tables)
					i, v = do_deserialize(s, i, tables)
					r[k] = v
				end
				return i+1, r
			end,
			["b"] = function(s, i, tables)
				local j, n = string.unpack(s, ">n", i)
				return j, tables[n]
			end
		})[string.sub(s, i, i)]
		if f == nil then
			error("Can't deserialize a \""..string.sub(s, i, i).."\" (at index "..i..").", 2)
		else
			return f(s, i+1, tables)
		end
	end
	local i, r = do_deserialize(s, 1, {n = 0})
	return r
end

--timer functions

function pipmak.schedule(dt, f, l)
	dt = pipmak.now() + math.max(0.001, tonumber(dt))
	local node = pipmak.thisnode()
	if node.firsttimer == nil or node.firsttimer.t > dt then
		node.firsttimer = { t = dt, f = f, l = l, n = node.firsttimer }
	else
		local temp = node.firsttimer
		while temp.n ~= nil and temp.n.t <= dt do temp = temp.n end
		temp.n = { t = dt, f = f, l = l, n = temp.n }
	end
end

function pipmak_internal.runtimers(now)
	local node = pipmak.thisnode()
	while node.firsttimer ~= nil and node.firsttimer.t <= now do
		local r, dt = pcall(node.firsttimer.f, now, node.firsttimer.l)
		if r then
			if dt ~= nil then
				pipmak.schedule(dt, node.firsttimer.f, now)
			end
		else
			pipmak.print("Error running scheduled function: ", dt);
		end
		node.firsttimer = node.firsttimer.n
	end
end

--node loading functions

function pipmak_internal.slide(properties)
	local image
	if type(properties) == "string" or getmetatable(properties) == pipmak_internal.meta_image then
		image = properties
		properties = {}
	elseif type(properties) == "table" then
		image = properties[1]
	else
		error("slide: table, string, or image expected, got " .. type(properties), 2)
	end
	if type(image) == "string" then
		image = pipmak.getimage(image)
	elseif getmetatable(image) ~= pipmak_internal.meta_image then
		error("slide background: string or image expected, got " .. type(image), 2)
	end
	local node = pipmak.thisnode()
	node.type = pipmak.slide
	setmetatable(node, pipmak_internal.meta_node)
	node.border = (properties.border == nil) and true or not(not(properties.border))
	node[1] = image
	if properties.cursor ~= nil then
		if getmetatable(properties.cursor) == pipmak_internal.meta_image then
			node:setstandardcursor(properties.cursor)
		else
			pipmak_internal.warning("cursor of slide: cursor expected, got " .. type(properties.cursor), 2);
		end
	end
	if properties.mousemode ~= nil then
		if properties.mousemode == pipmak.joystick or properties.mousemode == pipmak.direct then
			local mode = properties.mousemode
			local token
			onenternode(function() token = pipmak.pushmousemode(mode) end)
			onleavenode(function() pipmak.popmousemode(token) end)
		else
			pipmak_internal.warning("mousemode of slide: pipmak.joystick or pipmak.direct expected, got " .. type(properties.mousemode), 2)
		end
	end
	node.getcontrol = pipmak_internal.getcontrol_flat
end

function pipmak_internal.cubic(properties)
	if type(properties) ~= "table" then
		error("cube: table expected, got " .. type(image), 2)
	end
	local image
	local node = pipmak.thisnode()
	node.type = pipmak.cubic
	local flip = 0
	local flipbit = 1
	for i = 1, 6 do
		image = properties[i]
		if type(image) == "table" then
			if image.fliphorizontally then flip = flip + flipbit end
			if image.flipvertically then flip = flip + flipbit*2 end
			image = image[1]
		end
		if type(image) == "string" then
			image = pipmak.getimage(image)
		elseif getmetatable(image) ~= pipmak_internal.meta_image then
			error("cube face " .. i .. ": string or image expected, got " .. type(image), 2)
		end
		node[i] = image
		flipbit = flipbit * 4
	end
	node.flip = flip
	setmetatable(node, pipmak_internal.meta_node)
	if properties.cursor ~= nil then
		if getmetatable(properties.cursor) == pipmak_internal.meta_image then
			node:setstandardcursor(properties.cursor)
		else
			pipmak_internal.warning("cursor of cubic: cursor expected, got " .. type(properties.cursor), 2);
		end
	end
	if properties.mousemode ~= nil then
		if properties.mousemode == pipmak.joystick or properties.mousemode == pipmak.direct then
			local mode = properties.mousemode
			local token
			onenternode(function() token = pipmak.pushmousemode(mode) end)
			onleavenode(function() pipmak.popmousemode(token) end)
		else
			pipmak_internal.warning("mousemode of cubic: pipmak.joystick or pipmak.direct expected, got " .. type(properties.mousemode), 2)
		end
	end
	node.getcontrol = pipmak_internal.getcontrol_pano
	node.minaz = 0
	node.maxaz = 360
	node.minel = -80
	node.maxel = 80
end

function pipmak_internal.panel(properties)
	local image
	if type(properties) == "string" or getmetatable(properties) == pipmak_internal.meta_image then
		image = properties
		properties = {}
	elseif type(properties) == "table" then
		image = properties[1]
	else
		error("panel: table, string, or image expected, got " .. type(image), 2)
	end
	if type(image) == "string" then
		image = pipmak.getimage(image)
	elseif getmetatable(image) ~= pipmak_internal.meta_image then
		error("panel background: string or image expected, got " .. type(image), 2)
	end
	local node = pipmak.thisnode()
	node[1] = image
	node.type = pipmak.panel
	local iw, ih = image:size()
	local function optnumber(prop, default)
		local t = type(properties[prop])
		if t == "number" then return properties[prop]
		elseif t == "nil" then return default
		else error("property \"" .. prop .. "\" of panel: number or nil expected, got " .. t, 3) end
	end
	node.w = optnumber("w", iw)
	node.h = optnumber("h", ih)
	node.relx = optnumber("relx", properties.absx == nil and 0.5 or 0)
	node.rely = optnumber("rely", properties.absy == nil and 1/3 or 0)
	node.absx = optnumber("absx", -node.relx*node.w)
	node.absy = optnumber("absy", -node.rely*node.h)
	node.leftmargin = optnumber("leftmargin", nil)
	node.rightmargin = optnumber("rightmargin", nil)
	node.topmargin = optnumber("topmargin", nil)
	node.bottommargin = optnumber("bottommargin", nil)
	setmetatable(node, pipmak_internal.meta_panel)
	if properties.cursor ~= nil then
		if getmetatable(properties.cursor) == pipmak_internal.meta_image then
			node:setstandardcursor(properties.cursor)
		else
			pipmak_internal.warning("cursor of panel: cursor expected, got " .. type(properties.cursor), 2);
		end
	end
	if properties.mousemode ~= nil then
		if properties.mousemode == pipmak.joystick or properties.mousemode == pipmak.direct then
			local mode = properties.mousemode
			local token
			onenternode(function() token = pipmak.pushmousemode(mode) end)
			onleavenode(function() pipmak.popmousemode(token) end)
		else
			pipmak_internal.warning("mousemode of panel: pipmak.joystick or pipmak.direct expected, got " .. type(properties.mousemode), 2)
		end
	end
	node.getcontrol = pipmak_internal.getcontrol_flat
end

function pipmak_internal.hotspotmap(image)
	if type(image) == "string" then
		image = pipmak.getimage(image)
	end
	if getmetatable(image) ~= pipmak_internal.meta_image then
		pipmak_internal.warning("hotspotmap: string or image expected, got " .. type(image), 2)
	else
		pipmak.thisnode().hotspotmap = {
			[1] = image,
			type = ({
				[pipmak.slide] = pipmak.slide,
				[pipmak.panel] = pipmak.slide,
				[pipmak.cubic] = pipmak.equirect
			})[pipmak.thisnode().type]
		}
	end
end

function pipmak_internal.onkeydown_local(handler)
	if type(handler) == "function" then
		pipmak.thisnode().onkeydown = handler
	else
		pipmak_internal.warning("onkeydown: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.onenternode(handler)
	if type(handler) == "function" then
		table.insert(pipmak.thisnode().enternodehandlers, handler)
	else
		pipmak_internal.warning("onenternode: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.onleavenode(handler)
	if type(handler) == "function" then
		table.insert(pipmak.thisnode().leavenodehandlers, handler)
	else
		pipmak_internal.warning("onleavenode: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.onmouseenter(handler)
	if type(handler) == "function" then
		pipmak.thisnode().onmouseenter = handler
	else
		pipmak_internal.warning("onmouseenter: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.onmouseleave(handler)
	if type(handler) == "function" then
		pipmak.thisnode().onmouseleave = handler
	else
		pipmak_internal.warning("onmouseleave: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.hotspot(hotspot)
	if type(hotspot) ~= "table" then
		pipmak_internal.warning("hotspot: table expected, got " .. type(image), 2)
		return
	end
	if hotspot.cursor ~= nil and getmetatable(hotspot.cursor) ~= pipmak_internal.meta_image then
		pipmak_internal.warning("cursor of hotspot: cursor expected, got " .. type(hotspot.cursor), 2)
		hotspot.cursor = nil
	end
	for k, v in pairs({"onmousedown", "onmouseup", "onmouseenter", "onmouseleave", "onmousestilldown", "onmousewithin", "onhilite", "onenddrag"}) do
		if hotspot[v] ~= nil and type(hotspot[v]) ~= "function" then
			pipmak_internal.warning(v .. " handler of hotspot: function expected, got " .. type(hotspot[v]), 2)
			hotspot[v] = nil
		end
	end
	if hotspot.dont_pan == nil then
		hotspot.dont_pan = (hotspot.onmousestilldown ~= nil)
	end
	if hotspot.enabled == nil then
		hotspot.enabled = true
	end
	hotspot.file, hotspot.line = pipmak_internal.whereami()
	if hotspot.target ~= nil then
		if type(hotspot.target) ~= "number" and type(hotspot.target) ~= "string" then
			pipmak_internal.warning("target of hotspot: string or number expected, got " .. type(hotspot.target), 2)
		else
			if type(hotspot.effect) == "function" then
				hotspot.onmousedown = function(self)
					local ok, err = pcall(self.effect)
					if not ok then
						pipmak.print("Error running effect of hotspot " .. self.id .. ": " .. string.gsub(err, "to `%?' ", ""))
						return
					end
					pipmak.gotonode(self.target)
				end
			elseif type(hotspot.effect) == "table" then
				if type(hotspot.effect[1]) ~= "function" then
					pipmak_internal.warning("first item of effect of hotspot: function expected, got " .. type(hotspot.effect[1]), 2)
					hotspot.onmousedown = function(self)
						pipmak.gotonode(self.target)
					end
				else
					hotspot.onmousedown = function(self)
						local ok, err = pcall(unpack(self.effect))
						if not ok then
							pipmak.print("Error running effect of hotspot " .. self.id .. ": " .. string.gsub(err, "to `%?' ", ""))
							return
						end
						pipmak.gotonode(self.target)
					end
				end
			else
				if hotspot.effect ~= nil then
					pipmak_internal.warning("effect of hotspot: function or table expected, got " .. type(hotspot.effect), 2)
				end
				hotspot.onmousedown = function(self)
					pipmak.gotonode(self.target)
				end
			end
		end
	end
	local node = pipmak.thisnode()
	local i = node.lasthotspot + 1
	node.lasthotspot = i
	hotspot.id = i
	hotspot.type = pipmak.hotspot
	setmetatable(hotspot, pipmak_internal.meta_control)
	node.controls[i] = hotspot
	return hotspot
end

function pipmak_internal.handle(handle)
	if type(handle) ~= "table" then
		pipmak_internal.warning("handle: table expected, got " .. type(image), 2)
		return
	end
	local node = pipmak.thisnode()
	local propnames
	if math.mod(node.type, 2) == 1 then
		if handle.cx ~= nil then
			propnames = {"cx", "cy", "w", "h"}
		else
			propnames = {"x", "y", "w", "h"}
		end
	else
		if handle.caz ~= nil then
			propnames = {"caz", "cel", "w", "h"}
		else
			propnames = {"az", "el", "w", "h"}
		end
	end
	for k, v in pairs(propnames) do
		if type(handle[v]) ~= "number" then
			pipmak_internal.warning("property \"" .. v .. "\" of handle: number expected, got " .. type(handle[v]), 2)
			return
		end
	end
	if math.mod(node.type, 2) == 0 then
		if handle.caz ~= nil then
			handle.x = handle.caz - (handle.w / 2)
			handle.y = handle.cel + (handle.h / 2)
		else
			handle.x = handle.az
			handle.y = handle.el
		end
		handle.x = math.mod(handle.x, 360)
		if handle.x < 0 then
			handle.x = handle.x + 360
		end
	else
		if handle.cx ~= nil then
			handle.x = handle.cx - (handle.w / 2)
			handle.y = handle.cy - (handle.h / 2)
		end
	end
	if handle.cursor ~= nil and getmetatable(handle.cursor) ~= pipmak_internal.meta_image then
		pipmak_internal.warning("cursor of handle: cursor expected, got " .. type(handle.cursor), 2)
		handle.cursor = nil
	end
	for k, v in pairs({"onmousedown", "onmouseup", "onmouseenter", "onmouseleave", "onmousestilldown", "onmousewithin", "onhilite", "onenddrag"}) do
		if handle[v] ~= nil and type(handle[v]) ~= "function" then
			pipmak_internal.warning(v .. " handler of handle: function expected, got " .. type(handle[v]), 2)
			handle[v] = nil
		end
	end
	if handle.dont_pan == nil then
		handle.dont_pan = (handle.onmousestilldown ~= nil)
	end
	if handle.enabled == nil then
		handle.enabled = true
	end
	handle.file, handle.line = pipmak_internal.whereami()
	if handle.target ~= nil then
		if type(handle.target) ~= "number" and type(handle.target) ~= "string" then
			pipmak_internal.warning("target of handle: string or number expected, got " .. type(handle.target), 2)
		else
			if type(handle.effect) == "function" then
				handle.onmousedown = function(self)
					local ok, err = pcall(self.effect)
					if not ok then
						pipmak.print("Error running effect of handle " .. self.id .. ": " .. string.gsub(err, "to `%?' ", ""))
						return
					end
					pipmak.gotonode(self.target)
				end
			elseif type(handle.effect) == "table" then
				if type(handle.effect[1]) ~= "function" then
					pipmak_internal.warning("first item of effect of handle: function expected, got " .. type(handle.effect[1]), 2)
					handle.onmousedown = function(self)
						pipmak.gotonode(self.target)
					end
				else
					handle.onmousedown = function(self)
						local ok, err = pcall(unpack(self.effect))
						if not ok then
							pipmak.print("Error running effect of handle " .. self.id .. ": " .. string.gsub(err, "to `%?' ", ""))
							return
						end
						pipmak.gotonode(self.target)
					end
				end
			else
				if handle.effect ~= nil then
					pipmak_internal.warning("effect of handle: function or table expected, got " .. type(handle.effect), 2)
				end
				handle.onmousedown = function(self)
					pipmak.gotonode(self.target)
				end
			end
		end
	end
	local i = node.lasthandle + 1
	node.lasthandle = i
	handle.id = i - 255
	handle.type = pipmak.handle
	setmetatable(handle, pipmak_internal.meta_control)
	node.controls[i] = handle
	handle.node = node
	return handle
end

function pipmak_internal.patch(patch)
	if type(patch) ~= "table" then
		pipmak_internal.warning("patch: table expected, got " .. type(patch), 2)
		return
	end
	if patch.face == nil then
		patch.face = 1
	elseif type(patch.face) ~= "number" then
		pipmak_internal.warning("property \"face\" of patch: number expected, got " .. type(patch.face), 2)
		return
	end
	if patch.visible == nil then
		patch.visible = true
	end
	if type(patch.image) == "string" then
		patch.image = pipmak.getimage(patch.image)
	elseif getmetatable(patch.image) ~= pipmak_internal.meta_image then
		pipmak_internal.warning("property \"image\" of patch: string or image expected, got " .. type(patch.image), 2)
		return
	end
	local iw, ih = patch.image:size()
	if patch.w == nil then patch.w = iw end
	if patch.h == nil then patch.h = ih end
	local propnames = {"x", "y", "w", "h"}
	for k, v in pairs(propnames) do
		if patch["n"..v] ~= nil then v = "n"..v end
		if patch[v] == nil then
			pipmak_internal.warning("neither \"" .. v .. "\" nor \"n" .. v .."\" property given for patch", 2)
			return
		elseif type(patch[v]) ~= "number" then
			pipmak_internal.warning("property \"" .. v .. "\" of patch: number expected, got " .. type(patch[v]), 2)
			return
		end
	end
	propnames = {"nz", "anchorh", "anchorv", "angle", "anglex", "angley"}
	for k, v in pairs(propnames) do
		if patch[v] ~= nil and type(patch[v]) ~= "number" then
			pipmak_internal.warning("property \"" .. v .. "\" of patch: number or nil expected, got " .. type(patch[v]), 2)
			return
		end
	end
	patch.r = 1
	patch.g = 1
	patch.b = 1
	patch.a = 1
	patch.file, patch.line = pipmak_internal.whereami()
	local node = pipmak.thisnode()
	local i = node.lastpatch + 1
	node.lastpatch = i
	patch.id = i
	setmetatable(patch, pipmak_internal.meta_patch)
	node.patches[i] = patch
	return patch
end

function pipmak_internal.texteditor(properties)
	if type(properties) ~= "table" then
		pipmak_internal.warning("texteditor: table expected, got " .. type(properties), 2)
		return
	end
	for k, v in pairs({"x", "y", "w", "maxlength"}) do
		if type(properties[v]) ~= "number" then
			pipmak_internal.warning("property \"" .. v .. "\" of texteditor: number expected, got " .. type(properties[v]), 2)
			return
		end
	end
	local te = pipmak_internal.newtexteditor(properties.x, properties.y, properties.w, properties.maxlength)
	if properties.text ~= nil then
		if type(properties.text) == "string" then
			te:text(properties.text)
		else
			pipmak_internal.warning("property \"text\" of texteditor: string expected, got " .. type(properties.text), 2)
		end
	end
	if properties.onkeydown ~= nil then
		if type(properties.onkeydown) == "function" then
			te:setkeydownfunc(properties.onkeydown)
		else
			pipmak_internal.warning("property \"onkeydown\" of texteditor: function expected, got " .. type(properties.onkeydown), 2)
		end
	end
	local node = pipmak.thisnode()
	node.lasttexteditor = node.lasttexteditor + 1
	node.texteditors[node.lasttexteditor] = te
	return te
end

function pipmak_internal.sound(properties)
	if type(properties) == "string" then
		properties = { properties }
	elseif type(properties) ~= "table" then
		pipmak_internal.warning("sound: table or string expected, got " .. type(properties), 2)
		return
	end
	if type(properties[1]) ~= "string" then
		pipmak_internal.warning("file name of sound: string expected, got " .. type(properties[1]), 2)
		return
	end
	local ok, sound = pcall(pipmak.loadsound, properties[1])
	if not ok then
		pipmak_internal.warning(sound, 2)
		return
	end
	local err
	if properties.loop ~= nil then
		ok, err = pcall(sound.loop, sound, properties.loop)
		if not ok then pipmak_internal.warning("property \"loop\" of sound: " .. err, 2) end
	end
	if properties.volume ~= nil then
		ok, err = pcall(sound.volume, sound, properties.volume)
		if not ok then pipmak_internal.warning("property \"volume\" of sound: " .. err, 2) end
	end
	if properties.pitch ~= nil then
		ok, err = pcall(sound.pitch, sound, properties.pitch)
		if not ok then pipmak_internal.warning("property \"pitch\" of sound: " .. err, 2) end
	end
	if properties.az ~= nil then
		ok, err = pcall(sound.location, sound, properties.az, properties.el or 0)
		if not ok then pipmak_internal.warning("properties \"az\" and \"el\" of sound: " .. err, 2) end
	end
	if properties.autoplay then
		onenternode(function() sound:play() end)
		onleavenode(function() sound:stop() end)
	end
	return sound
end

function pipmak_internal.messages(messages)
	if type(messages) ~= "table" then
		pipmak_internal.warning("messages: table expected, got " .. type(messages), 2)
		return
	end
	for k, v in pairs(messages) do
		if type(k) ~= "string" then
			pipmak_internal.warning("message \"" .. tostring(k) .. "\": message names must be strings", 2)
		elseif type(v) ~= "function" then
			pipmak_internal.warning("message \"" .. k .. "\": function expected, got " .. type(v), 2)
		else
			pipmak.thisnode().messages[k] = v
		end
	end
end

function pipmak_internal.limits(limits)
	if type(limits) ~= "table" then
		pipmak_internal.warning("limits: table expected, got " .. type(limits), 2)
		return
	end
	local node = pipmak.thisnode()
	for i, v in ipairs({"minaz", "maxaz", "minel", "maxel"}) do
		if type(limits[v]) == "number" then
			node[v] = limits[v]
		elseif limits[v] ~= nil then
			pipmak_internal.warning("limit \"" .. v .. "\": number expected, got " .. type(limits[v]), 2)
		end
	end
	
end

function pipmak_internal.getcontrol_flat(node, mouseH, mouseV, hotspot)
	if hotspot ~= 0 and node.controls[hotspot].enabled then return hotspot end
	local h
	for i = node.lasthandle, 256, -1 do
		h = node.controls[i]
		if h.enabled and mouseH >= h.x and mouseH < h.x + h.w and mouseV >= h.y and mouseV < h.y + h.h then
			return i
		end
	end
	return 0
end

function pipmak_internal.getcontrol_pano(node, mouseH, mouseV, hotspot)
	if hotspot ~= 0 and node.controls[hotspot].enabled then return hotspot end
	local h
	for i = node.lasthandle, 256, -1 do
		h = node.controls[i]
		if h.enabled and ((mouseH >= h.x and mouseH < h.x + h.w) or mouseH < h.x + h.w - 360) and mouseV <= h.y and mouseV > h.y - h.h then
			return i
		end
	end
	return 0
end

local function updatepanelbbox(panel)
	local minx, maxx, miny, maxy = 0, panel.w, 0, panel.h
	local h
	for i = panel.lasthandle, 256, -1 do
		h = panel.controls[i]
		if h.x < minx then minx = h.x end
		if h.x + h.w > maxx then maxx = h.x + h.w end
		if h.y < miny then miny = h.y end
		if h.y + h.h > maxy then maxy = h.y + h.h end
	end
	pipmak_internal.setpanelbbox(panel, minx, maxx, miny, maxy)
end

function pipmak_internal.loadnode(node)
	
	slide = pipmak_internal.slide
	cubic = pipmak_internal.cubic
	panel = pipmak_internal.panel
	hotspotmap = pipmak_internal.hotspotmap
	onkeydown = pipmak_internal.onkeydown_local
	onenternode = pipmak_internal.onenternode
	onleavenode = pipmak_internal.onleavenode
	onmouseenter = pipmak_internal.onmouseenter
	onmouseleave = pipmak_internal.onmouseleave
	hotspot = pipmak_internal.hotspot
	handle = pipmak_internal.handle
	patch = pipmak_internal.patch
	texteditor = pipmak_internal.texteditor
	sound = pipmak_internal.sound
	messages = pipmak_internal.messages
	limits = pipmak_internal.limits
	
	-- node only contains cnode and path at this time
	node.controls = {}
	node.lasthotspot = 0
	node.lasthandle = 255
	node.patches = {}
	node.lastpatch = 0
	node.texteditors = {}
	node.lasttexteditor = 0
	node.messages = {}
	node.enternodehandlers = {}
	node.leavenodehandlers = {}
	
	local ok = pipmak_internal.dofile("/" .. node.path .. "/node.lua")
	
	if ok and node.type == nil then
		pipmak.print("Error loading node " .. node.path .. ": No node type set")
		ok = false
	end
	
	if ok and node.type == pipmak.panel then
		updatepanelbbox(node)
	end
	
	slide = nil
	cubic = nil
	panel = nil
	hotspotmap = nil
	onkeydown = nil
	onenternode = nil
	onleavenode = nil
	onmouseenter = nil
	onmouseleave = nil
	hotspot = nil
	handle = nil
	patch = nil
	texteditor = nil
	sound = nil
	messages = nil
	limits = nil
	
	return ok
	
end

function pipmak_internal.title(title)
	--The title string will be passed to SDL_WM_SetCaption() which does not specify what string encoding it accepts. On Mac OS X, it wants UTF-8 and hangs when it gets an invalid UTF-8 string. On Windows, it seems to want Windows-Latin-1. Therefore, ASCII-ize the string to be safe.
	pipmak_internal.project.title = string.gsub(tostring(title), "[^ -~]", "?")
end

function pipmak_internal.startnode(nodeid)
	if type(nodeid) ~= "number" and type(nodeid) ~= "string" then
			error("startnode: string or number expected, got " .. type(nodeid), 2)
	else
		nodeid = tostring(nodeid)
		if nodeid == "0" or string.sub(nodeid, 1, 1) == "-" then
			error("startnode: node names \"0\" and starting with \"-\" are reserved for internal use", 2)
		end
		pipmak_internal.project.startnode = nodeid
	end
end

function pipmak_internal.onopenproject(handler)
	if type(handler) == "function" then
		pipmak_internal.project.onopenproject = handler
	else
		pipmak_internal.warning("onopenproject: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.onkeydown_global(handler)
	if type(handler) == "function" then
		pipmak_internal.project.onkeydown = handler
	else
		pipmak_internal.warning("onkeydown: function expected, got " .. type(handler), 2)
	end
end

function pipmak_internal.loadmain(defaultsonly)
	
	title = pipmak_internal.title
	startnode = pipmak_internal.startnode
	version = pipmak_internal.version
	onopenproject = pipmak_internal.onopenproject
	onkeydown = pipmak_internal.onkeydown_global
	
	pipmak_internal.project = {
		title = "Pipmak",
		startnode = 0,
		prefsnode = 0,
		onkeydown = function(key)
			local f = ({
				[string.byte("m")] = function()
					if (pipmak.getmousemode() == pipmak.joystick) then
						pipmak.setmousemode(pipmak.direct)
					else
						pipmak.setmousemode(pipmak.joystick)
					end
				end,
				[string.byte("f")] = pipmak.setfullscreen,
				[string.byte("d")] = function()
					pipmak.setfullscreen(true)
				end,
				[string.byte("w")] = pipmak.setwindowed,
				[string.byte("i")] = function()
					pipmak.setinterpolation(not pipmak.getinterpolation())
				end,
				[string.byte("r")] = function()
					pipmak.flushimagecache()
					pipmak.dissolve(1, 0)
					pipmak.gotonode("/" .. pipmak.getcurrentnode())
				end,
				[string.byte("c")] = function()
					pipmak.setshowcontrols(not pipmak.getshowcontrols())
				end,
				[string.byte("s")] = pipmak.savegame,
				[string.byte("o")] = pipmak.opensavedgame,
				[string.byte("p")] = function()
					pipmak.dissolve()
					pipmak.openproject()
				end,
				[string.byte("1")] = function()
					local s = pipmak.getjoystickspeed()/2
					pipmak.setjoystickspeed(s)
					pipmak.print("Joystick Speed = ", s, " degree/(sec pixel)")
				end,
				[string.byte("2")] = function()
					local s = pipmak.getjoystickspeed()*2
					pipmak.setjoystickspeed(s)
					pipmak.print("Joystick Speed = ", s, " degree/(sec pixel)")
				end,
				[string.byte("x")] = function()
					local f = pipmak.getverticalfov() + 5
					if f > 95 then f = 95 end
					pipmak.setverticalfov(f)
					pipmak.print("FOV = ", f, " degrees (vertical)")
				end,
				[string.byte("z")] = function()
					local f = pipmak.getverticalfov() - 5
					if f < 5 then f = 5 end
					pipmak.setverticalfov(f)
					pipmak.print("FOV = ", f, " degrees (vertical)")
				end,
				[string.byte("n")] = function()
					pipmak.print("We're at node \"", pipmak.getcurrentnode(), "\"")
				end,
				[pipmak.key_esc] = function()
					pipmak.dissolve()
					pipmak.overlaynode("/0", true)
				end,
				[string.byte("q")] = pipmak.quit,
				[string.byte("e")] = pipmak_internal.openfile,
				[string.byte("l")] = function()
					pipmak.overlaynode("/-11")
				end
			})[key]
			if f == nil then
				pipmak.print("Unused key ", key, " pressed.");
			else
				f()
			end
		end
	}
	
	pipmak.flushimagecache()
	
	if not defaultsonly then
		pipmak_internal.dofile("/main.lua")
	end
	
	title = nil
	startnode = nil
	version = nil
	onopenproject = nil
	onkeydown = nil
	
end

pipmak_internal.loadmain(true) --set project defaults right here to avoid special cases when opening a saved game from the command line before any project is open

--control methods

function pipmak_internal.meta_control.__tostring(self)
	if self.type == pipmak.hotspot then
		return "hotspot " .. self.id
	else
		return "handle " .. self.id
	end
end

function pipmak_internal.meta_control.getid(self)
	return self.id
end

function pipmak_internal.meta_control.moveto(self, x, y)
	if self.type ~= pipmak.handle then
		error("`moveto' can only be called on handles", 2)
	end
	if type(x) ~= "number" then
		error("bad argument #1 to `moveto' (number expected, got " .. type(x) .. ")", 2)
	end
	if type(y) ~= "number" then
		error("bad argument #2 to `moveto' (number expected, got " .. type(y) .. ")", 2)
	end
	self.x = x;
	self.y = y;
	if self.node.type == pipmak.panel then updatepanelbbox(self.node) end
end

function pipmak_internal.meta_control.move(self, props)
	if self.type ~= pipmak.handle then
		error("`move' can only be called on handles", 2)
	end
	for k, v in pairs({"az", "el", "x", "y", "w", "h"}) do
		if props[v] ~= nil then
			if type(props[v]) ~= "number" then
				error("bad argument `" .. v .. "' to `move' (number expected, got " .. type(props[v]) .. ")", 2)
			end
			self[({"x", "y", "x", "y", "w", "h"})[k]] = props[v]
		end
	end
	if self.node.type == pipmak.panel then updatepanelbbox(self.node) end
end

function pipmak_internal.meta_control.location(self)
	if self.type ~= pipmak.handle then
		error("`location' can only be called on handles", 2)
	end
	return self.x, self.y
end

function pipmak_internal.meta_control.enable(self, en)
	self.enabled = en
end

function pipmak_internal.meta_control.isenabled(self)
	return self.enabled
end


--patch methods

function pipmak_internal.meta_patch.__tostring(self)
	return "patch " .. self.id
end

function pipmak_internal.meta_patch.getid(self)
	return self.id
end

function pipmak_internal.meta_patch.location(self)
	return self.x, self.y, self.face
end

function pipmak_internal.meta_patch.moveto(self, x, y, face)
	self.x = x
	self.y = y
	if face ~= nil then
		self.face = face
	end
	pipmak_internal.updatepatch(self)
end

function pipmak_internal.meta_patch.move(self, props)
	if props.face ~= nil then self.face = props.face end
	if props.nx ~= nil then
		self.nx = props.nx
	elseif props.x ~= nil then
		self.x = props.x
		self.nx = nil
	end
	if props.ny ~= nil then
		self.ny = props.ny
	elseif props.y ~= nil then
		self.y = props.y
		self.ny = nil
	end
	if props.nz ~= nil then self.nz = props.nz end
	if props.nw ~= nil then
		self.nw = props.nw
	elseif props.w ~= nil then
		self.w = props.w
		self.nw = nil
	end
	if props.nh ~= nil then
		self.nh = props.nh
	elseif props.h ~= nil then
		self.h = props.h
		self.nh = nil
	end
	if props.anchorh ~= nil then self.anchorh = props.anchorh end
	if props.anchorv ~= nil then self.anchorv = props.anchorv end
	if props.angle ~= nil then self.angle = props.angle end
	if props.anglex ~= nil then self.anglex = props.anglex end
	if props.angley ~= nil then self.angley = props.angley end
	pipmak_internal.updatepatch(self)
end

function pipmak_internal.meta_patch.isvisible(self)
	return self.visible
end

function pipmak_internal.meta_patch.setvisible(self, visible)
	self.visible = not(not(visible))
	pipmak_internal.updatepatch(self)
end

function pipmak_internal.meta_patch.getcolor(self)
	return self.r, self.g, self.b, self.a
end

function pipmak_internal.meta_patch.setcolor(self, red, green, blue, alpha)
	if type(red) ~= "number" or red < 0 or red > 1 then
		error("bad argument #1 to `setcolor' (red: expected number between 0 and 1)", 2)
	end
	if type(green) ~= "number" or green < 0 or green > 1 then
		error("bad argument #2 to `setcolor' (green: expected number between 0 and 1)", 2)
	end
	if type(blue) ~= "number" or blue < 0 or blue > 1 then
		error("bad argument #3 to `setcolor' (blue: expected number between 0 and 1)", 2)
	end
	if alpha ~= nil and (type(alpha) ~= "number" or alpha < 0 or alpha > 1) then
		error("bad argument #4 to `setcolor' (alpha: expected number between 0 and 1)", 2)
	end
	self.r = red
	self.g = green
	self.b = blue
	if alpha ~= nil then
		self.a = alpha
	end
	pipmak_internal.updatepatch(self)
end

function pipmak_internal.meta_patch.getalpha(self)
	return self.a
end

function pipmak_internal.meta_patch.setalpha(self, alpha)
	if type(alpha) ~= "number" or alpha < 0 or alpha > 1 then
		error("bad argument #1 to `setalpha' (alpha: expected number between 0 and 1)", 2)
	end
	self.a = alpha
	pipmak_internal.updatepatch(self)
end

function pipmak_internal.meta_patch.setimage(self, image)
	-- keep a reference to the old image until after updatepatch(), otherwise a garbage collection coming between self.image = ... and updatepatch() will collect it while its texture is still in use
	local oldimage = self.image
	if type(image) == "string" then
		self.image = pipmak.getimage(image)
	elseif getmetatable(image) ~= pipmak_internal.meta_image then
		error("bad argument to `setimage' (string or image expected, got " .. type(image) .. ")", 2)
	else
		self.image = image
	end
	pipmak_internal.updatepatch(self)
end


--node methods

function pipmak_internal.meta_node.__tostring(self)
	if self.path ~= nil then return "node \"" .. self.path .. "\""
	else return "meta_node" end
	-- the check is necessary because otherwise tostring(pipmak_internal.meta_panel) would cause an error (we should actually return something like "table: 0x123", but using tostring(self) here would cause an infinite recursion...)
end

function pipmak_internal.meta_node.getname(self)
	return self.path
end

pipmak_internal.meta_node.getid = pipmak_internal.meta_node.getname -- for backwards compatibility


--panel methods

pipmak_internal.meta_panel.__tostring = pipmak_internal.meta_node.__tostring -- tostring uses rawget to look for __tostring in the metatable, so it isn't inherited

function pipmak_internal.meta_panel.moveto(self, relx, rely, absx, absy, duration)
	local function optnumber(arg, n, default)
		local t = type(n)
		if t == "number" then return n
		elseif t == "nil" then return default
		else error("bad argument #" .. arg .. " to `moveto' (number or nil expected, got " .. t .. ")", 3) end
	end
	self.relx = optnumber(1, relx, absx == nil and 0.5 or 0)
	self.rely = optnumber(2, rely, absy == nil and 1/3 or 0)
	self.absx = optnumber(3, absx, -self.relx*self.w)
	self.absy = optnumber(4, absy, -self.rely*self.h)
	duration = optnumber(5, duration, 0)
	if duration < 0 then duration = 0 end
	pipmak_internal.updatepanelposition(self, duration)
end

function pipmak_internal.meta_panel.moveby(self, dx, dy)
	local sw, sh = pipmak.screensize()
	local x, y = self.relx*sw + self.absx, self.rely*sh + self.absy
	local rx, ry = x/(sw - self.w), y/(sh - self.h)
	rx = rx <= 0.2 and 0 or rx >= 0.8 and 1 or (rx - 0.2)/0.6
	ry = ry <= 0.2 and 0 or ry >= 0.8 and 1 or (ry - 0.2)/0.6
	self:moveto(rx, ry, x + dx - rx*sw, y + dy - ry*sh)
end

function pipmak_internal.meta_panel.location(self)
	return self.relx, self.rely, self.absx, self.absy
end
