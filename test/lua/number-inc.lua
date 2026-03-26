require("plugins/number-inc-dec")
local win = vis.win

describe("between numbers", function()
	it("between numbers", function()
		win.selection.pos = 4
		vis:feedkeys("<C-a>")
		assert.are.equal("1.2rc4", win.file.lines[1])
	end)
end)

describe("in number", function()
	it("enclosed number", function()
		win.selection.pos = 11
		vis:feedkeys("<C-a>")
		assert.are.equal("1xx20xx", win.file.lines[2])
	end)

	it("enclosed number", function()
		win.selection.pos = 10
		vis:feedkeys("<C-a>")
		assert.are.equal("1xx21xx", win.file.lines[2])
	end)

	it("end of word", function()
		win.selection.pos = 24
		vis:feedkeys("<C-a>")
		assert.are.equal("1foo22bar4", win.file.lines[3])
	end)

	it("first digit word start", function()
		win.selection.pos = 26
		vis:feedkeys("<C-a>")
		assert.are.equal("12y", win.file.lines[4])
	end)

	it("last digit word start", function()
		win.selection.pos = 27
		vis:feedkeys("<C-a>")
		assert.are.equal("13y", win.file.lines[4])
	end)

	it("first digit single number", function()
		win.selection.pos = 30
		vis:feedkeys("<C-a>")
		assert.are.equal("20", win.file.lines[5])
	end)

	it("last digit single number", function()
		win.selection.pos = 31
		vis:feedkeys("<C-x>")
		assert.are.equal("19", win.file.lines[5])
	end)
end)
