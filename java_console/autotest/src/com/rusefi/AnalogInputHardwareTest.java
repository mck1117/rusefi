package com.rusefi;

import com.rusefi.config.generated.Fields;
import static com.rusefi.IoUtil.getDisableCommand;
import static com.rusefi.binaryprotocol.BinaryProtocol.sleep;

public class PwmHardwareTest extends RusefiTestBase {
	void setIdlePositionAndAssertTps(int idle, int expectedTps) {
		ecu.sendCommand("set idle_position " + pct);

		float actualTps = SensorCentral.getInstance().getValue(Sensor.TPS);

		// Accept up to 2% error
		assertEquals(expectedTps, actualTps, 0.02);
	}

	@Test
	public void testAnalogInput() {
		ecu.setEngineType(61); // proteus
		ecu.sendCommand(getDisableCommand(Fields.CMD_SELF_STIMULATION));

		// Let the ECU set itself up for this test
		ecu.sendCommand("set_analog_input_test_pwm");

		// 0% duty -> failed TPS (voltage too low)
		setIdlePositionAndAssertTps(0, 0);

		// These should all be valid points
		setIdlePositionAndAssertTps(25, 0);
		setIdlePositionAndAssertTps(50, 50);
		setIdlePositionAndAssertTps(75, 100);

		// 100% duty -> failed TPS (voltage too high)
		setIdlePositionAndAssertTps(100, 0);
	}
}
