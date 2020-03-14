#pragma once

struct closed_loop_fuel_cfg
{
	float lambdaDeadband;
};

class ClosedLoopFuelCellBase {
public:
	void update();
	float getAdjustment() const;

protected:
	// Helpers - virtual for mocking
	virtual float getLambdaError() const = 0;
	virtual float getLambdaDeadband() const = 0;
	virtual float getMaxAdjustment() const = 0;
	virtual float getMinAdjustment() const = 0;
	virtual float getAdjustmentRate() const = 0;

private:
	// Current fueling adjustment.
	// 0 = no adjustment.
	// 0.1 = add 10% fuel.
	float m_adjustment = 0;
};

class ClosedLoopFuelCellImpl final : public ClosedLoopFuelCellBase {
public:
	void configure(const closed_loop_fuel_cfg* configuration) {
		m_config = configuration;
	}

private:
	const closed_loop_fuel_cfg* m_config = nullptr;

protected:
	float getLambdaError() const override;
	float getLambdaDeadband() const override;
	float getMaxAdjustment() const override;
	float getMinAdjustment() const override;
	float getAdjustmentRate() const override;
};
