
vis:map(vis.modes.NORMAL, "K", function()
	vis:feedkeys("iNormal Mode<Escape>")
end)

vis:map(vis.modes.INSERT, "K", function()
	vis:feedkeys("Insert Mode<Escape>")
end)

vis:map(vis.modes.VISUAL, "K", function()
	vis:feedkeys("<Escape>iVisual Mode<Escape>")
end)

vis:map(vis.modes.VISUAL_LINE, "K", function()
	vis:feedkeys("<Escape>iVisual Line Mode<Escape>")
end)

vis:map(vis.modes.REPLACE, "K", function()
	vis:feedkeys("Replace Mode<Escape>")
end)

local win = vis.win
local file = win.file

describe("map", function()

	before_each(function()
		win.selection.pos = 0
	end)

	after_each(function()
		file:delete(0, file.size)
	end)

	local same = function(expected)
		local data = file:content(0, file.size)
		assert.are.same(expected, data)
	end

	it("normal mode", function()
		vis:feedkeys("K")
		same("Normal Mode")
	end)

	it("insert mode", function()
		vis:feedkeys("iK")
		same("Insert Mode")
	end)

	it("visual mode", function()
		vis:feedkeys("vK")
		same("Visual Mode")
	end)

	it("visual line mode", function()
		vis:feedkeys("VK")
		same("Visual Line Mode")
	end)

	it("replace mode", function()
		vis:feedkeys("RK")
		same("Replace Mode")
	end)
end)
