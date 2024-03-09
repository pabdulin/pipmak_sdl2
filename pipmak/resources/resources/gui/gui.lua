-- Windoid
--------------------------------------------------------------------------------

local
function windoid(title, width, height, more)
	if type(more) ~= "table" then more = {} end
	
	-- window background
	panel { "../resources/gui/windoid.png", w = width+16, h = height+16, leftmargin = 8.5, rightmargin = 8.5, topmargin = 22.5, bottommargin = 16.5, cursor = pipmak.arrow, relx = more.relx, absx = more.absx, rely = more.rely, absy = more.absy }
	
	-- window title
	patch { x = 8, y = 9, image = pipmak.newimage(width, 12):drawtext(width/2, 0, title, "../resources/VeraBd.ttf", 9, pipmak.center) }
	
	-- draggable title bar
	handle {
		x = 8, y = 8, w = width, h = 14,
		onmousestilldown = function()
			pipmak.thisnode():moveby(pipmak.mouseminusclickloc())
		end,
		onmousedown = function()
			--if this panel is overlaid, bring it to the front
			if pipmak.thisnode() ~= pipmak.backgroundnode() then
				pipmak.overlaynode(pipmak.thisnode():getid())
			end
		end
	}
	
	-- close box
	local closebox = patch { x = 10, y = 8, image = "../resources/gui/closebox.png" }
	handle {
		x = 10, y = 8, w = 14, h = 14,
		onhilite = function(self, h) closebox:setimage(h and "../resources/gui/closebox_h.png" or "../resources/gui/closebox.png") end,
		onmouseup = function() pipmak.thisnode():closeoverlay() end
	}
	
end


-- Scroll Bar
--------------------------------------------------------------------------------

local meta_scrollbar = {

	update = function(self)
		if (self.window < self.range) then
			self.thumbStartY = self.trackStartY + self.value*self.pixelsPerIncrement
			self.thumbEndY = self.trackStartY + (self.value + self.window)*self.pixelsPerIncrement
		else
			self.thumbStartY = self.trackStartY
			self.thumbEndY = self.trackEndY
		end
		self.thumbP:move { y = self.thumbStartY, h = self.thumbEndY - self.thumbStartY }
		self.thumbH:move { y = self.thumbStartY, h = self.thumbEndY - self.thumbStartY }
		self.pageUpH:move { y = self.trackStartY, h = self.thumbStartY - self.trackStartY }
		self.pageDownH:move { y = self.thumbEndY, h = self.trackEndY - self.thumbEndY }
	end,
	
	enable = function(self, enabled)
		self.upArrowH:enable(enabled)
		self.downArrowH:enable(enabled)
		self.pageUpH:enable(enabled)
		self.pageDownH:enable(enabled)
		self.thumbH:enable(enabled)
		self.upArrowP:setalpha(enabled and 1 or 0.3)
		self.downArrowP:setalpha(enabled and 1 or 0.3)
		self.thumbP:setvisible(enabled)
	end,
	
	setrange = function(self, range)
		self.range = range
		if self.value < 0 then
			self.value = 0
		elseif self.value > self.range - self.window then
			self.value = self.range - self.window
		end
		self.pixelsPerIncrement = (self.height - 22)/self.range
		self:update()
		self:enable(self.window < self.range)
	end,
	
	setvalue = function(self, value)
		local oldvalue = self.value
		self.value = value
		if self.value < 0 then
			self.value = 0
		elseif self.value > self.range - self.window then
			self.value = self.range - self.window
		end
		local changed = (self.value ~= oldvalue)
		if changed then self:update() end
		return changed
	end,
	
	scrollBy = function(self, delta)
		if self:setvalue(self.value + delta) then
			self.onscroll(self.value)
		end
	end
}

meta_scrollbar.__index = meta_scrollbar

local
function scrollbar(props)

	local self = {}
	setmetatable(self, meta_scrollbar)
	
	self.pageIncrement = props.window - 1
	if self.pageIncrement < 1 then self.pageIncrement = 1 end
	self.value = props.value or 0
	self.window = props.window
	self.range = props.range
	self.height = props.h
	self.onscroll = props.onscroll
	self.pixelsPerIncrement = (props.h - 22)/props.range
	self.trackStartY = props.y + 11
	self.trackEndY = props.y + props.h - 11
	self.scrolling = false
	
	self.upArrowP = patch { x = props.x, y = props.y - 1, image = "../resources/gui/triangle_up.png" }
	self.downArrowP = patch { x = props.x, y = props.y + props.h - 12, image = "../resources/gui/triangle_down.png" }
	
	patch { -- track
		x = props.x + 5, y = self.trackStartY, w = 5, h = self.trackEndY - self.trackStartY,
		image = pipmak.newimage(1, 1):color(0, 0, 0, .1):fill()
	}
	
	self.thumbP = patch {
		x = props.x + 5, y = 0, w = 5, h = 0,
		image = pipmak.newimage(1, 1):color(0, 0, 0, .555):fill()
	}
	
	self.upArrowH = handle {
		x = props.x, y = props.y, w = 15, h = 11,
		onhilite = function(hdl, hi)
			self.upArrowP:setimage(hi and "../resources/gui/triangle_up_h.png" or "../resources/gui/triangle_up.png")
			self.scrolling = hi
			if hi then
				self:scrollBy(-1)
				pipmak.schedule(0.5,
					function()
						if not self.scrolling then return nil end
						self:scrollBy(-1)
						return 0.1
					end
				)
			end
		end
	}
	
	self.downArrowH = handle {
		x = props.x, y = self.trackEndY, w = 15, h = 11,
		onhilite = function(hdl, hi)
			self.downArrowP:setimage(hi and "../resources/gui/triangle_down_h.png" or "../resources/gui/triangle_down.png")
			self.scrolling = hi
			if hi then
				self:scrollBy(1)
				pipmak.schedule(0.5,
					function()
						if not self.scrolling then return nil end
						self:scrollBy(1)
						return 0.1
					end
				)
			end
		end
	}
	
	self.pageUpH = handle {
		x = props.x, y = 0, w = 15, h = 0,
		onhilite = function(hdl, hi)
			self.scrolling = hi
			if hi then
				self:scrollBy(-self.pageIncrement)
				pipmak.schedule(0.5,
					function()
						if not self.scrolling then return nil end
						self:scrollBy(-self.pageIncrement)
						return 0.1
					end
				)
			end
		end
	}
	
	self.pageDownH = handle {
		x = props.x, y = 0, w = 15, h = 0,
		onhilite = function(hdl, hi)
			self.scrolling = hi
			if hi then
				self:scrollBy(self.pageIncrement)
				pipmak.schedule(0.5,
					function()
						if not self.scrolling then return nil end
						self:scrollBy(self.pageIncrement)
						return 0.1
					end
				)
			end
		end
	}
	
	self.thumbH = handle {
		x = props.x, y = 0, w = 15, h = 0,
		onmousedown = function()
			self.thumbClickValue = self.value
		end,
		onmousestilldown = function()
			local dx, dy = pipmak.mouseminusclickloc()
			self:scrollBy(self.thumbClickValue + math.floor(dy/self.pixelsPerIncrement + 0.5) - self.value)
		end
	}
	
	self:update()
	self:enable(self.window < self.range)
	
	return self
	
end

--------------------------------------------------------------------------------

return {
	windoid = windoid,
	scrollbar = scrollbar
}
