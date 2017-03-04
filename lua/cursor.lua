local busted = require 'busted.runner'()

local win = vis.win

-- check that cursor position remains unchanged after an invalid adjustment
local invalid_pos = function(place)
	local pos = win.cursor.pos
	local line = win.cursor.line
	local col = win.cursor.col
	win.cursor:to(line, col)
	assert.has_error(place)
	assert.are.equal(pos, win.cursor.pos)
	assert.are.equal(line, win.cursor.line)
	assert.are.equal(col, win.cursor.col)
end

describe("win.cursor", function()

	it("initial position", function()
		assert.are.equal(0, win.cursor.pos)
	end)

	it("initial line", function()
		assert.are.equal(1, win.cursor.line)
	end)

	it("initial column", function()
		assert.are.equal(1, win.cursor.col)
	end)
end)

describe("win.cursor.pos", function()
	
	it("= 0", function()
		win.cursor.pos = 0
		assert.are.equal(0, win.cursor.pos)
		assert.are.equal(1, win.cursor.line)
		assert.are.equal(1, win.cursor.col)
	end)

	it("= beyond end of file", function()
		win.cursor.pos = win.file.size
		local pos = win.cursor.pos
		local line = win.cursor.line
		local col = win.cursor.col
		win.cursor.pos = 0
		-- cursor is placed on last valid position
		win.cursor.pos = win.file.size+1
		assert.are.equal(pos, win.cursor.pos)
		assert.are.equal(line, win.cursor.line)
		assert.are.equal(col, win.cursor.col)
	end)

end)

describe("win.cursor.to", function()

	it("(5, 3)", function()
		win.cursor:to(5, 3)
		assert.are.equal(30, win.cursor.pos)
		assert.are.equal(5, win.cursor.line)
		assert.are.equal(3, win.cursor.col)
	end)

	it("(0, 0) invalid position", function()
		-- is that what we want?
		win.cursor:to(0, 0)
		assert.are.equal(0, win.cursor.pos)
		assert.are.equal(1, win.cursor.line)
		assert.are.equal(1, win.cursor.col)
	end)

	it("invalid position, negative line", function()
		invalid_pos(function() win.cursor:to(-1, 0) end)
	end)

	it("invalid position, negative column", function()
		invalid_pos(function() win.cursor:to(0, -1) end)
	end)

	it("invalid position, non-integer line", function()
		invalid_pos(function() win.cursor:to(1.5, 1) end)
	end)

	it("invalid position, non-integer column", function()
		invalid_pos(function() win.cursor:to(1, 1.5) end)
	end)

--[[
	it("move beyond end of file", function()
		win.cursor.pos = win.file.size
		local pos = win.cursor.pos
		local line = win.cursor.line
		local col = win.cursor.col
		win.cursor.pos = 0
		-- cursor is placed on last valid position
		win.cursor:to(#win.file.lines+2, 1000)
		assert.are.equal(pos, win.cursor.pos)
		assert.are.equal(line, win.cursor.line)
		assert.are.equal(col, win.cursor.col)
	end)
--]]

end)

