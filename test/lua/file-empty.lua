
local file = vis.win.file

describe("empty file", function()

	it("has size zero", function()
		assert.are.equal(0, file.size)
	end)

	it("has zero lines", function()
		assert.are.equal(0, #file.lines)
	end)

	it("has lines[1] == ''", function()
		-- is that what we want?
		assert.are.equal("", file.lines[1])
	end)

end)

