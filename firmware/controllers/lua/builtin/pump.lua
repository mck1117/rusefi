function pumpLogic()
	prime = getUptime() > getConfig(0)
	spinning = getTimeSinceTrigger() < 1

	return prime or spinning
end
