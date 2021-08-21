function getcfg(cfg, index)
	return 3
end

function getSensor(idx)
	return 75
end

function setpin(idx, val) end

function makeFan(fanIndex)
	retval = {
		idx = fanIndex,
		state = false
	}

	function retval:update()
		lo = getcfg(0, self.fanIndex)
		hi = getcfg(1, self.fanIndex)
		disableWhenStopped = getcfg(2, self.fanIndex)
		clt = getSensor(1)
		ac = isAcOn()

		if isCranking() then
			-- inhibit while cranking
			self.state = false;
		elseif disableWhenStopped and not isRunning() then
			-- inhibit while not running (if so configured)
			self.state = false
		elseif clt == nil then
			-- If CLT is broken, turn the fan on
			self.state = true
		elseif enableWithAc and ac then
			-- Enable with AC if so configured
			self.state = true
		elseif clt > hi then
			-- hot, enable
			self.state = true
		elseif clt < lo then
			-- cold, disable
			self.state = false
		end

		-- return result
		return self.state
	end

	return retval
end
