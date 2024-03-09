--------------------------------------------------------------------------
-- serialize.lua
--
-- Functions to serialize Lua values into binary strings and reconstruct
-- them from such strings. Serializable values are nil, numbers, strings,
-- booleans, and tables containing only serializable keys and values.
-- Tables containing themselves or multiple references to the same table
-- are handled properly. For the binary encoding, the 'pack' extension by
-- Luiz Henrique de Figueiredo is used.
--
-- Run this file for a demonstration.
--
-- Differences to Steve Dekorte's 'Pickle' functions:
--  o The serialized data is binary, not human-readable.
--  o Deserialization is not done by the Lua interpreter, so it is not
--    possible to cause side effects by embedding arbitrary Lua code in
--    the serialized data.
--
-- Christian Walther, June 2004
-- Public Domain - feel free to use the contents of this file, including
-- the auxiliary functions, for whatever you like.
--------------------------------------------------------------------------


-- auxiliary functions for the demo ----

function hexdump(s, w)
	if w == nil then w = 16 end
	w = math.min(math.floor(w), string.len(s))
	local l,h
	local r = ""
	local i = 0
	local ol = string.len(string.format("%X", string.len(s)-1))
	repeat
		l = string.sub(s, i*w+1, (i+1)*w)
		h = string.gsub(l, ".", function(x) return string.format("%02X ", string.byte(x)) end)
		if string.len(h) ~= 3*w then h = string.format("%-"..3*w.."s", h) end
		l = string.gsub(l, "[^ -~]", ".")
		r = r..string.format("0x%0"..ol.."X | ", i*w)..h.."| "..l.."\n"
		i = i+1
	until i*w+1 > string.len(s)
	return string.sub(r, 1, string.len(r)-1)
end

function tabletostring(t, d, tables)
	local function mytostring(v)
		if type(v) == "string" then return string.format("%q", v)
		else return tostring(v)
		end
	end
	if type(t) == "table" then
		if tables == nil then tables = {} end
		if tables[t] == nil then
			tables[t] = 1
			if d == nil then d = 0 end
			local r = "("..tostring(t)..") = {\n"
			for k, v in pairs(t) do
				r = r..string.rep("\t", d+1).."["..mytostring(k).."] = "..tabletostring(v, d+1, tables).."\n"
			end
			return r..string.rep("\t", d).."}"
		else
			return "("..mytostring(t)..")"
		end
	else
		return mytostring(t)
	end
end


-- serialization functions ----

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
			error("Can't serialize a " .. type(d) .. ".")
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
			error("Can't deserialize a \""..string.sub(s, i, i).."\" (at index "..i..").")
		else
			return f(s, i+1, tables)
		end
	end
	local i, r = do_deserialize(s, 1, {n = 0})
	return r
end


-- demonstration ----

testtable = {
	1,
	3,
	key = "value",
	[18.5] = true,
	[false] = 3.141
}
testtable2 = {3, 4, 5, ["loop"] = testtable}
testtable.t = testtable2
testtable[testtable2] = "that's a table"

print("\nOriginal table:\n")
print(tabletostring(testtable))
s = serialize(testtable)
print("\nSerialized:\n")
print(hexdump(s))
print("\nDeserialized:\n")
print(tabletostring(deserialize(s)))
