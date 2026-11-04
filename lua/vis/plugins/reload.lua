
-- Reload can only get you so far, we can't wipe vis[] itself

local loadedoninit = {} for k,v in pairs(package.loaded) do
	loadedoninit[k] = true
end

local Command = function(argv)
	local loaded = package.loaded
	local name = argv[1]
	if name then
		loaded[name] = nil
	else -- reloads everything
		loaded.visrc = nil
		for NAME in pairs(loaded) do
			if NAME:find'^vis[/.]' or loadedoninit[NAME] then
				loaded[NAME] = nil
			end
		end
		name = 'visrc'
	end
	return pcall(require(name))
end

vis:command_register('reload', Command, 'Reloads lua package')
