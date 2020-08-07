#pragma once

template <int TrackingLength>
class GapTracker {
public:
	static constexpr int _len = TrackingLength;
	GapTracker() {
		reset();
	}

	void trackTooth(efitick_t nowNt) {
		auto gap = nowNt - m_lastToothTime;

		auto gapRatio = gap / m_lastGap;

		// scoot the array over 1 - newest ratio at the head
		for (size_t i = _len - 1; i >= 0; i--) {
			m_gapRatios[i + 1] = m_gapRatios[i];
		}

		m_gapRatios[0] = gapRatio;
		m_lastGap = gap;
		m_lastToothTime = nowNt;
	}

	bool isMatch(const float (&testGaps)[_len], float ratioMin, float ratioMax) const {
		for (size_t i = 0; i < _len; i++) {
			auto target = testGaps[i];

			// Target of 0 indicates that we're done - all previous checks have passed
			if (target == 0) {
				return true;
			}

			auto test = m_gapRatios[i];
			
			// A zero indicates that we don't have enough teeth tracked yet
			if (test == 0) {
				return false;
			}

			auto minAllowed = target * ratioMin;
			auto maxAllowed = target * ratioMax;


			// if the tested gap is outside the allowed range, it's not a match.
			if (test < minAllowed || test > maxAllowed) {
				return false;
			}
		}

		// No gap was out of spec - it's a match!
		return true;
	}

	float getLastGap() const {
		return m_gapRatios[0];
	}

	void reset() {
		for (size_t i = 0; i < _len; i++) {
			m_gapRatios[i] = 0;
		}

		m_lastToothTime = -1;
		m_lastGap = 0;
	}

private:
	efitick_t m_lastToothTime = -1;
	efitick_t m_lastGap = 0;
	float m_gapRatios[_len];
};
