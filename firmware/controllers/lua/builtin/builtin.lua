
-- initialize systems
fan1 = makeFan(0)
fan2 = makeFan(1)

-- Slow callback: called at 20hz
function periodicSlowCallback()
	fan1:update()
	fan2:update()
end
