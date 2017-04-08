require 'busted.runner'()

local file = vis.win.file

describe("file.lines_iterator()", function()

	it("same as file.lines[]", function()
		local i = 1
		local iterator_lines = {}
		for line in file:lines_iterator() do
			iterator_lines[i] = line
			i = i + 1
		end
		local array_lines = {}
		for j = 1, #file.lines do
			array_lines[j] = file.lines[j]
		end
		assert.are.same(array_lines, iterator_lines)
	end)
end)

describe("get file.lines[]", function()

	it("#lines", function()
		assert.are.equals(5, #file.lines)
	end)

	it("get lines[0]", function()
		-- is that what we want?
		assert.are.equals(file.lines[0], file.lines[1])
	end)

	it("get lines[1]", function()
		assert.are.equals("1", file.lines[1])
	end)

	it("get empty \n line", function()
		assert.are.equals("", file.lines[2])
	end)

	it("get empty \r\n line", function()
		assert.are.equals("\r", file.lines[4])
	end)

	it("get lines[#lines]", function()
		assert.are.equals("5", file.lines[#file.lines])
	end)

	it("get lines[#lines+1]", function()
		-- is that what we want?
		assert.are.equals("5", file.lines[#file.lines])
	end)

end)

describe("set file.lines[]", function()

	it("replace empty \n line", function()
		local new = "line 2"
		file.lines[2] = new
		assert.are.equals(new, file.lines[2])
	end)

--[[
	it("replace empty \r\n line", function()
		local new = "line 4"
		file.lines[4] = new
		assert.are.equals(new, file.lines[4])
	end)
--]]

	it("set lines[0], add new line at start", function()
		local lines = #file.lines
		local new_first = "new first line"
		local old_first = file.lines[1]
		file.lines[0] = new_first
		assert.are.equals(lines+1, #file.lines)
		assert.are.equals(new_first, file.lines[1])
		assert.are.equals(old_first, file.lines[2])
	end)

	it("set lines[#lines+1], add new line at end", function()
		local lines = #file.lines
		local new_last = "new last line"
		local old_last = file.lines[#file.lines]
		file.lines[#file.lines+1] = new_last
		assert.are.equals(lines+1, #file.lines)
		assert.are.equals(new_last, file.lines[#file.lines])
		assert.are.equals(old_last, file.lines[lines])
	end)

end)

