#pragma once


class ClosedLoopFuelCellBase {
public:
	void update(float adjustRate, float lambdaDeadband);
	float getAdjustment() const;

protected:
	// Helpers - virtual for mocking
	virtual float getLambdaError() const = 0;
	virtual float getMaxAdjustment() const = 0;
	virtual float getMinAdjustment() const = 0;

private:
	// Current fueling adjustment.
	// 0 = no adjustment.
	// 0.1 = add 10% fuel.
	float m_adjustment = 0;
};

struct stft_cell_cfg_s;

class ClosedLoopFuelCellImpl final : public ClosedLoopFuelCellBase {
public:
	void configure(const stft_cell_cfg_s* configuration) {
		m_config = configuration;
	}

private:
	const stft_cell_cfg_s *m_config = nullptr;

protected:
	float getLambdaError() const override;
	float getMaxAdjustment() const override;
	float getMinAdjustment() const override;
};
