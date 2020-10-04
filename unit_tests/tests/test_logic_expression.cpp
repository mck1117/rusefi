/**
 * @file test_logic_expression.cpp
 *
 * https://sourceforge.net/p/rusefi/tickets/102/
 *
 * @date Oct 3, 2014
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "fsio_impl.h"
#include "cli_registry.h"
#include "engine_test_helper.h"
#include "thermistors.h"
#include "allsensors.h"

#define TEST_POOL_SIZE 256

float getEngineValue(le_action_e action DECLARE_ENGINE_PARAMETER_SUFFIX) {
	switch(action) {
	case LE_METHOD_FAN:
		return engine->fsioState.mockFan;
	case LE_METHOD_COOLANT:
		return Sensor::get(SensorType::Clt).value_or(0);
	case LE_METHOD_RPM:
		return engine->fsioState.mockRpm;
	case LE_METHOD_CRANKING_RPM:
		return engine->fsioState.mockCrankingRpm;
	case LE_METHOD_TIME_SINCE_BOOT:
		return engine->fsioState.mockTimeSinceBoot;
	case LE_METHOD_VBATT:
		return 12;
	case LE_METHOD_AC_TOGGLE:
		return getAcToggle(PASS_ENGINE_PARAMETER_SIGNATURE);
	case LE_METHOD_IS_COOLANT_BROKEN:
		return 0;
#include "fsio_getters.def"
	default:
	firmwareError(OBD_PCM_Processor_Fault, "FSIO: No mock value for %d", action);
		return NAN;
	}
}

TEST(fsio, testParsing) {
	char buffer[64];

	ASSERT_TRUE(strEqualCaseInsensitive("hello", "HELlo"));
	ASSERT_FALSE(strEqualCaseInsensitive("hello", "HElo2"));

	const char *ptr;
	ptr = getNextToken("  hello  ", buffer, sizeof(buffer));
	ASSERT_TRUE(strEqual("hello", buffer));

	ptr = getNextToken("hello", buffer, sizeof(buffer));
	ASSERT_TRUE(strEqual("hello", buffer));

	ptr = getNextToken("  hello  world ", buffer, sizeof(buffer));
	ASSERT_TRUE(strEqual("hello", buffer));
	ptr = getNextToken(ptr, buffer, sizeof(buffer));
	ASSERT_TRUE(strEqual("world", buffer));

	ASSERT_TRUE(isNumeric("123"));
	ASSERT_FALSE(isNumeric("a123"));

	LEElement thepool[TEST_POOL_SIZE];
	LEElementPool pool(thepool, TEST_POOL_SIZE);

	LEElement *element;
	element = pool.parseExpression("1 3 AND not");
	ASSERT_TRUE(element != NULL);

	ASSERT_EQ(element->action, LE_NUMERIC_VALUE);
	ASSERT_EQ(element->fValue, 1.0);

	element = element->next;
	ASSERT_EQ(element->action, LE_NUMERIC_VALUE);
	ASSERT_EQ(element->fValue, 3.0);

	element = element->next;
	ASSERT_EQ(element->action, LE_OPERATOR_AND);

	element = element->next;
	ASSERT_EQ(element->action, LE_OPERATOR_NOT);

	element = element->next;
	ASSERT_TRUE(element == NULL);
}

static void testExpression2(float selfValue, const char *line, float expected, Engine *engine) {
	LEElement thepool[TEST_POOL_SIZE];
	LEElementPool pool(thepool, TEST_POOL_SIZE);
	LEElement * element = pool.parseExpression(line);
	print("Parsing [%s]\n", line);
	ASSERT_TRUE(element != NULL) << "Not NULL expected";
	LECalculator c;

	EXPAND_Engine;

	ASSERT_NEAR(expected, c.getValue2(selfValue, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan(), EPS4D) << line;
}

static void testExpression2(float selfValue, const char *line, float expected, const std::unordered_map<SensorType, float>& sensorVals = {}) {
	WITH_ENGINE_TEST_HELPER_SENS(FORD_INLINE_6_1995, sensorVals);
	testExpression2(selfValue, line, expected, engine);
}

static void testExpression(const char *line, float expectedValue, const std::unordered_map<SensorType, float>& sensorVals = {}) {
	testExpression2(0, line, expectedValue, sensorVals);
}

TEST(fsio, testIfFunction) {
	testExpression("true 22 33 if", 22);
}

TEST(fsio, testHysteresisSelf) {
	WITH_ENGINE_TEST_HELPER(FORD_INLINE_6_1995);

	LEElement thepool[TEST_POOL_SIZE];
	LEElementPool pool(thepool, TEST_POOL_SIZE);
	// value ON: 450
	// value OFF: 400
	// Human formula: (self and (rpm > 400)) | (rpm > 450)
	LEElement * element = pool.parseExpression("self rpm 400 > and rpm 450 > |");
	ASSERT_TRUE(element != NULL) << "Not NULL expected";

	LECalculator c;
	double selfValue = 0;

	engine->fsioState.mockRpm = 0;
	selfValue = c.getValue2(selfValue, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan();
	ASSERT_EQ(0, selfValue);

	engine->fsioState.mockRpm = 430;
	selfValue = c.getValue2(selfValue, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan();
	// OFF since not ON yet
	ASSERT_EQ(0, selfValue);

	engine->fsioState.mockRpm = 460;
	selfValue = c.getValue2(selfValue, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan();
	ASSERT_EQ(1, selfValue);

	engine->fsioState.mockRpm = 430;
	selfValue = c.getValue2(selfValue, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan();
	// OFF since was ON yet
	ASSERT_EQ(1, selfValue);
}

TEST(fsio, testLogicExpressions) {
	{

	WITH_ENGINE_TEST_HELPER(FORD_INLINE_6_1995);


	testExpression("123", 123.0f);

	// Test basic operations
	testExpression("123 456 +", 579);
	testExpression("123 456 -", -333);
	testExpression("123 456 *", 56088);
	testExpression("123 456 /", 0.269737f);

	// Boolean operators
	testExpression("true true and", 1);
	testExpression("true false and", 0);
	testExpression("true false or", 1);
	testExpression("false false or", 0);

	LECalculator c;

	LEElement value1;
	value1.init(123.0f);
	c.add(&value1);

	assertEqualsM("123", 123.0, c.getValue(0 PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan());

	LEElement value2;
	value2.init(321.0f);
	c.add(&value2);

	LEElement value3;
	value3.init(LE_OPERATOR_AND);
	c.add(&value3);
	assertEqualsM("123 and 321", 1.0, c.getValue(0 PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan());

	/**
	 * fuel_pump = (time_since_boot < 4 seconds) OR (rpm > 0)
	 * fuel_pump = time_since_boot 4 less rpm 0 > OR
	 */

	c.reset();

	LEElement thepool[TEST_POOL_SIZE];
	LEElementPool pool(thepool, TEST_POOL_SIZE);
	LEElement *e = pool.next();
	e->init(LE_METHOD_TIME_SINCE_BOOT);

	e = pool.next();
	e->init(4.0f);

	e = pool.next();
	e->init(LE_OPERATOR_LESS);

	e = pool.next();
	e->init(LE_METHOD_RPM);

	e = pool.next();
	e->init(0.0f);

	e = pool.next();
	e->init(LE_OPERATOR_MORE);

	e = pool.next();
	e->init(LE_OPERATOR_OR);

	pool.reset();
	LEElement *element;
	element = pool.parseExpression("fan no_such_method");
	ASSERT_TRUE(element == NULL) << "NULL expected";

	}

	/**
	 * fan = (not fan && coolant > 90) OR (fan && coolant > 85)
	 * fan = fan NOT coolant 90 AND more fan coolant 85 more AND OR
	 */

	std::unordered_map<SensorType, float> sensorVals = {{SensorType::Clt, 100}};
	testExpression("coolant 1 +", 101, sensorVals);

	testExpression("fan", 0, sensorVals);
	testExpression("fan not", 1, sensorVals);
	testExpression("coolant 90 >", 1, sensorVals);
	testExpression("fan not coolant 90 > and", 1, sensorVals);

	testExpression("100 200 1 if", 200);
	testExpression("10 99 max", 99);

	testExpression2(123, "10 self max", 123);

	testExpression("fan NOT coolant 90 > AND fan coolant 85 > AND OR", 1, sensorVals);

	{
		WITH_ENGINE_TEST_HELPER_SENS(FORD_INLINE_6_1995, sensorVals);
		LEElement thepool[TEST_POOL_SIZE];
		LEElementPool pool(thepool, TEST_POOL_SIZE);
		LEElement * element = pool.parseExpression("fan NOT coolant 90 > AND fan coolant 85 > AND OR");
		ASSERT_TRUE(element != NULL) << "Not NULL expected";
		LECalculator c;
		ASSERT_EQ( 1,  c.getValue2(0, element PASS_ENGINE_PARAMETER_SUFFIX).asFloatWithNan()) << "that expression";

		ASSERT_EQ(12, c.currentCalculationLogPosition);
		ASSERT_EQ(102, c.calcLogAction[0]);
		ASSERT_EQ(0, c.calcLogValue[0].Value);
	}


	testExpression("0 1 &", 0);
	testExpression("0 1 |", 1);

	testExpression("0 1 >", 0);

	{
		WITH_ENGINE_TEST_HELPER_SENS(FORD_INLINE_6_1995, sensorVals);
		engineConfiguration->fanOnTemperature = 0;
		engineConfiguration->fanOffTemperature = 0;

		testExpression2(0, "cfg_fanOffTemperature", 0, engine);
		testExpression2(0, FAN_CONTROL_LOGIC, 1, engine);
		testExpression2(0, "coolant cfg_fanOffTemperature >", 1, engine);
	}

	{
		WITH_ENGINE_TEST_HELPER_SENS(FORD_INLINE_6_1995, sensorVals);
		engine->fsioState.mockRpm = 900;
		engine->fsioState.mockCrankingRpm = 200;
		testExpression2(0, "rpm", 900, engine);
		testExpression2(0, "cranking_rpm", 200, engine);
		testExpression2(0, STARTER_RELAY_LOGIC, 0, engine);
		testExpression2(0, "rpm cranking_rpm > ", 1, engine);
	}
}
