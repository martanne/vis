local file = vis.win.file

describe("vis.pipe", function()

	local FULLSCREEN = true

	it("no input", function()
		vis:pipe("cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal("", f:read("*a"))
		f:close()
		os.remove("f")
	end)

	it("no input fullscreen", function()
		vis:pipe("cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal("", f:read("*a"))
		f:close()
		os.remove("f")
	end)

	it("buffer", function()
		vis:pipe("foo", "cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("buffer fullscreen", function()
		vis:pipe("foo", "cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("range", function()
		vis:pipe(file, {start=0, finish=3}, "cat > f")
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("range fullscreen", function()
		vis:pipe(file, {start=0, finish=3}, "cat > f", FULLSCREEN)
		local f = io.open("f", "r")
		assert.truthy(f)
		assert.are.equal(f:read("*a"), "foo")
		f:close()
		os.remove("f")
	end)

	it("explicit nil text", function()
		assert.has_error(function() vis:pipe(nil, "true") end)
	end)

	it("explicit nil text fullscreen", function()
		assert.has_error(function() vis:pipe(nil, "true", FULLSCREEN) end)
	end)

	it("explicit nil file", function()
		assert.has_error(function() vis:pipe(nil, {start=0, finish=0}, "true") end)
	end)

	it("explicit nil file fullscreen", function()
		assert.has_error(function() vis:pipe(nil, {start=0, finish=0}, "true", FULLSCREEN) end)
	end)

	it("wrong argument order file, range, cmd", function()
		assert.has_error(function() vis:pipe({start=0, finish=0}, vis.win.file, "true") end)
	end)

	it("wrong argument order fullscreen, cmd", function()
		assert.has_error(function() vis:pipe(FULLSCREEN, "true") end)
	end)
end)
