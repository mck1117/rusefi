/**
 * @file	HIP9011.h
 * @brief	HIP9011/TPIC8101 driver
 *
 * @date Nov 27, 2013
 * @author Andrey Belomutskiy, (c) 2012-2017
 */

#ifndef HIP9011_H_
#define HIP9011_H_


// 0b01000000
#define SET_PRESCALER_CMD 0x40

// 0b11100000
#define SET_CHANNEL_CMD 0xE0

// 0b11000000
#define SET_INTEGRATOR_CMD 0xC0

// 0b00000000
#define SET_BAND_PASS_CMD 0x0

// 0b10000000
#define SET_GAIN_CMD 0x80

// 0b01110001
#define SET_ADVANCED_MODE 0x71

#define HIP_THREAD_PERIOD 100

void initHip9011(Logging *sharedLogger);
void setHip9011FrankensoPinout(void);
#if HAL_USE_ADC
void hipAdcCallback(adcsample_t value);
#endif /* HAL_USE_ADC */
void setHipGain(float value);
void setHipBand(float value);
void setPrescalerAndSDO(int value);
void setKnockThresh(float value);
void setMaxKnockSubDeg(int value);

#endif /* HIP9011_H_ */
