
local win = vis.win

-- check that selection position remains unchanged after an invalid adjustment
local invalid_pos = function(place)
	local pos = win.selection.pos
	local line = win.selection.line
	local col = win.selection.col
	win.selection:to(line, col)
	assert.has_error(place)
	assert.are.equal(pos, win.selection.pos)
	assert.are.equal(line, win.selection.line)
	assert.are.equal(col, win.selection.col)
end

describe("win.selection", function()

	it("initial position", function()
		assert.are.equal(0, win.selection.pos)
	end)

	it("initial line", function()
		assert.are.equal(1, win.selection.line)
	end)

	it("initial column", function()
		assert.are.equal(1, win.selection.col)
	end)
end)

describe("win.selection.pos", function()

	it("= 0", function()
		win.selection.pos = 0
		assert.are.equal(0, win.selection.pos)
		assert.are.equal(1, win.selection.line)
		assert.are.equal(1, win.selection.col)
	end)

	it("= beyond end of file", function()
		win.selection.pos = win.file.size
		local pos = win.selection.pos
		local line = win.selection.line
		local col = win.selection.col
		win.selection.pos = 0
		-- selection is placed on last valid position
		win.selection.pos = win.file.size+1
		assert.are.equal(pos, win.selection.pos)
		assert.are.equal(line, win.selection.line)
		assert.are.equal(col, win.selection.col)
	end)

end)

describe("win.selection.to", function()

	it("(5, 3)", function()
		win.selection:to(5, 3)
		assert.are.equal(30, win.selection.pos)
		assert.are.equal(5, win.selection.line)
		assert.are.equal(3, win.selection.col)
	end)

	it("(0, 0) invalid position", function()
		-- is that what we want?
		win.selection:to(0, 0)
		assert.are.equal(0, win.selection.pos)
		assert.are.equal(1, win.selection.line)
		assert.are.equal(1, win.selection.col)
	end)

	it("invalid position, negative line", function()
		invalid_pos(function() win.selection:to(-1, 0) end)
	end)

	it("invalid position, negative column", function()
		invalid_pos(function() win.selection:to(0, -1) end)
	end)

	it("invalid position, non-integer line", function()
		invalid_pos(function() win.selection:to(1.5, 1) end)
	end)

	it("invalid position, non-integer column", function()
		invalid_pos(function() win.selection:to(1, 1.5) end)
	end)

--[[
	it("move beyond end of file", function()
		win.selection.pos = win.file.size
		local pos = win.selection.pos
		local line = win.selection.line
		local col = win.selection.col
		win.selection.pos = 0
		-- selection is placed on last valid position
		win.selection:to(#win.file.lines+2, 1000)
		assert.are.equal(pos, win.selection.pos)
		assert.are.equal(line, win.selection.line)
		assert.are.equal(col, win.selection.col)
	end)
--]]

end)

