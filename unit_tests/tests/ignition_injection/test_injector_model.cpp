#include "engine_test_helper.h"
#include "injector_model.h"
#include "mocks.h"

#include "gtest/gtest.h"

using ::testing::StrictMock;

class MockInjectorModel : public InjectorModelBase {
public:
	MOCK_METHOD(floatms_t, getDeadtime, (), (const, override));
	MOCK_METHOD(float, getInjectorMassFlowRate, (), (const, override));
};

TEST(InjectorModel, Prepare) {
	StrictMock<MockInjectorModel> dut;

	EXPECT_CALL(dut, getDeadtime());
	EXPECT_CALL(dut, getInjectorMassFlowRate());

	dut.prepare();
}

TEST(InjectorModel, getInjectionDuration) {
	StrictMock<MockInjectorModel> dut;

	EXPECT_CALL(dut, getDeadtime())
		.WillOnce(Return(2.0f));
	EXPECT_CALL(dut, getInjectorMassFlowRate())
		.WillOnce(Return(4.8f)); // 400cc/min

	dut.prepare();

	EXPECT_NEAR(dut.getInjectionDuration(0.01f), 10 / 4.8f + 2.0f, EPS4D);
	EXPECT_NEAR(dut.getInjectionDuration(0.02f), 20 / 4.8f + 2.0f, EPS4D);
}

TEST(InjectorModel, Deadtime) {
	WITH_ENGINE_TEST_HELPER(TEST_ENGINE);

	// Some test data in the injector correction table
	for (size_t i = 0; i < efi::size(engineConfiguration->injector.battLagCorr); i++) {
		CONFIG(injector.battLagCorr)[i] = 2 * i;
		CONFIG(injector.battLagCorrBins)[i] = i;
	}

	InjectorModel dut;
	INJECT_ENGINE_REFERENCE(&dut);

	engine->sensors.vBatt = 3;
	EXPECT_EQ(dut.getDeadtime(), 6);

	engine->sensors.vBatt = 7;
	EXPECT_EQ(dut.getDeadtime(), 14);
}

TEST(InjectorModel, sensorBasedPressure) {
	WITH_ENGINE_TEST_HELPER(TEST_ENGINE);

	InjectorModel dut;
	INJECT_ENGINE_REFERENCE(&dut);

	// todo

	EXPECT_EQ(100,  dut.getInjectorDifferentialPressure();
}
