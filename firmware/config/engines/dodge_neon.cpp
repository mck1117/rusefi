/**
 * @file	dodge_neon.cpp
 *
 * DODGE_NEON_1995 = 2
 * set engine_type 2
 *
 * DODGE_NEON_2003 = 23
 * set engine_type 23
 * http://rusefi.com/wiki/index.php?title=Vehicle:Dodge_Neon_2003
 *
 * @date Dec 16, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "global.h"

#include "dodge_neon.h"
#include "engine_configuration.h"
#include "thermistors.h"
#include "engine_math.h"
#include "fsio_impl.h"
#include "allsensors.h"

#include "custom_engine.h"

#define xxxxx 12

static const fsio_table_8x8_f32t vBattTarget = {
		/* Generated by TS2C on Sat Jul 02 16:13:34 EDT 2016*/
		{/* 0 20.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 1 30.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 2 40.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 3 60.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 4 70.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 5 90.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 6 100.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
		{/* 7 120.000	*//* 0 650.0*/12.000,	/* 1 800.0*/12.000,	/* 2 1650.0*/13.000,	/* 3 2500.0*/14.000,	/* 4 3350.0*/15.000,	/* 5 4200.0*/15.000,	/* 6 5050.0*/15.000,	/* 7 7000.0*/15.000,	},
};

#if IGN_LOAD_COUNT == DEFAULT_IGN_LOAD_COUNT
static const ignition_table_t fromODB = {
/* Generated by OBD2C on Tue Jun 30 18:47:52 EDT 2015*/
{/* 0 0.000	*//* 0 800.0*/10.000,	/* 1 1213.0*/22.000,	/* 2 1626.0*/22.000,	/* 3 2040.0*/22.000,	/* 4 2453.0*/24.000,	/* 5 2866.0*/22.000,	/* 6 3280.0*/23.000,	/* 7 3693.0*/23.000,	/* 8 4106.0*/22.000,	/* 9 4520.0*/22.000,	/* 10 4933.0*/22.000,	/* 11 5346.0*/32.000,	/* 12 5760.0*/22.000,	/* 13 6173.0*/22.000,	/* 14 6586.0*/20.000,	/* 15 7000.0*/20.000,	},
{/* 1 6.667	*//* 0 800.0*/10.000,	/* 1 1213.0*/22.000,	/* 2 1626.0*/22.000,	/* 3 2040.0*/22.000,	/* 4 2453.0*/24.000,	/* 5 2866.0*/22.000,	/* 6 3280.0*/22.500,	/* 7 3693.0*/22.500,	/* 8 4106.0*/22.500,	/* 9 4520.0*/22.500,	/* 10 4933.0*/22.500,	/* 11 5346.0*/32.000,	/* 12 5760.0*/22.500,	/* 13 6173.0*/22.000,	/* 14 6586.0*/20.000,	/* 15 7000.0*/20.000,	},
{/* 2 13.333	*//* 0 800.0*/10.000,	/* 1 1213.0*/22.000,	/* 2 1626.0*/22.500,	/* 3 2040.0*/22.861,	/* 4 2453.0*/24.243,	/* 5 2866.0*/22.806,	/* 6 3280.0*/24.270,	/* 7 3693.0*/23.280,	/* 8 4106.0*/21.143,	/* 9 4520.0*/19.857,	/* 10 4933.0*/18.000,	/* 11 5346.0*/32.667,	/* 12 5760.0*/22.500,	/* 13 6173.0*/22.500,	/* 14 6586.0*/22.500,	/* 15 7000.0*/20.000,	},
{/* 3 20.000	*//* 0 800.0*/10.000,	/* 1 1213.0*/22.897,	/* 2 1626.0*/21.810,	/* 3 2040.0*/21.643,	/* 4 2453.0*/22.500,	/* 5 2866.0*/19.400,	/* 6 3280.0*/13.750,	/* 7 3693.0*/26.000,	/* 8 4106.0*/35.000,	/* 9 4520.0*/41.214,	/* 10 4933.0*/45.125,	/* 11 5346.0*/52.750,	/* 12 5760.0*/51.000,	/* 13 6173.0*/43.000,	/* 14 6586.0*/40.000,	/* 15 7000.0*/40.000,	},
{/* 4 26.667	*//* 0 800.0*/11.912,	/* 1 1213.0*/17.390,	/* 2 1626.0*/26.200,	/* 3 2040.0*/32.950,	/* 4 2453.0*/35.111,	/* 5 2866.0*/40.000,	/* 6 3280.0*/49.711,	/* 7 3693.0*/45.273,	/* 8 4106.0*/43.500,	/* 9 4520.0*/49.000,	/* 10 4933.0*/46.176,	/* 11 5346.0*/52.500,	/* 12 5760.0*/45.500,	/* 13 6173.0*/50.333,	/* 14 6586.0*/40.000,	/* 15 7000.0*/40.000,	},
{/* 5 33.333	*//* 0 800.0*/7.373,	/* 1 1213.0*/17.786,	/* 2 1626.0*/26.923,	/* 3 2040.0*/39.900,	/* 4 2453.0*/29.500,	/* 5 2866.0*/49.306,	/* 6 3280.0*/49.861,	/* 7 3693.0*/43.056,	/* 8 4106.0*/44.708,	/* 9 4520.0*/45.800,	/* 10 4933.0*/43.375,	/* 11 5346.0*/52.000,	/* 12 5760.0*/48.667,	/* 13 6173.0*/50.000,	/* 14 6586.0*/40.000,	/* 15 7000.0*/40.000,	},
{/* 6 40.000	*//* 0 800.0*/8.224,	/* 1 1213.0*/19.667,	/* 2 1626.0*/35.318,	/* 3 2040.0*/35.846,	/* 4 2453.0*/39.857,	/* 5 2866.0*/40.250,	/* 6 3280.0*/43.000,	/* 7 3693.0*/37.750,	/* 8 4106.0*/42.250,	/* 9 4520.0*/36.167,	/* 10 4933.0*/43.000,	/* 11 5346.0*/44.250,	/* 12 5760.0*/47.500,	/* 13 6173.0*/49.000,	/* 14 6586.0*/40.000,	/* 15 7000.0*/40.000,	},
{/* 7 46.667	*//* 0 800.0*/8.930,	/* 1 1213.0*/26.429,	/* 2 1626.0*/30.750,	/* 3 2040.0*/35.700,	/* 4 2453.0*/41.000,	/* 5 2866.0*/32.667,	/* 6 3280.0*/30.500,	/* 7 3693.0*/39.500,	/* 8 4106.0*/38.000,	/* 9 4520.0*/30.167,	/* 10 4933.0*/41.583,	/* 11 5346.0*/44.000,	/* 12 5760.0*/45.500,	/* 13 6173.0*/49.000,	/* 14 6586.0*/49.000,	/* 15 7000.0*/40.000,	},
{/* 8 53.333	*//* 0 800.0*/7.568,	/* 1 1213.0*/28.611,	/* 2 1626.0*/29.850,	/* 3 2040.0*/32.667,	/* 4 2453.0*/27.333,	/* 5 2866.0*/26.500,	/* 6 3280.0*/43.000,	/* 7 3693.0*/29.250,	/* 8 4106.0*/39.400,	/* 9 4520.0*/39.500,	/* 10 4933.0*/41.000,	/* 11 5346.0*/37.667,	/* 12 5760.0*/42.750,	/* 13 6173.0*/45.500,	/* 14 6586.0*/46.500,	/* 15 7000.0*/40.000,	},
{/* 9 60.000	*//* 0 800.0*/7.500,	/* 1 1213.0*/18.000,	/* 2 1626.0*/23.938,	/* 3 2040.0*/30.929,	/* 4 2453.0*/24.500,	/* 5 2866.0*/23.000,	/* 6 3280.0*/39.167,	/* 7 3693.0*/32.000,	/* 8 4106.0*/29.333,	/* 9 4520.0*/30.000,	/* 10 4933.0*/40.000,	/* 11 5346.0*/37.000,	/* 12 5760.0*/40.000,	/* 13 6173.0*/43.250,	/* 14 6586.0*/40.000,	/* 15 7000.0*/40.000,	},
{/* 10 66.667	*//* 0 800.0*/8.000,	/* 1 1213.0*/17.200,	/* 2 1626.0*/23.917,	/* 3 2040.0*/25.200,	/* 4 2453.0*/23.000,	/* 5 2866.0*/18.000,	/* 6 3280.0*/34.000,	/* 7 3693.0*/30.250,	/* 8 4106.0*/28.000,	/* 9 4520.0*/31.000,	/* 10 4933.0*/38.000,	/* 11 5346.0*/37.000,	/* 12 5760.0*/40.000,	/* 13 6173.0*/42.000,	/* 14 6586.0*/43.000,	/* 15 7000.0*/51.500,	},
{/* 11 73.333	*//* 0 800.0*/15.250,	/* 1 1213.0*/13.600,	/* 2 1626.0*/15.583,	/* 3 2040.0*/23.100,	/* 4 2453.0*/21.500,	/* 5 2866.0*/25.750,	/* 6 3280.0*/29.750,	/* 7 3693.0*/31.750,	/* 8 4106.0*/28.000,	/* 9 4520.0*/31.000,	/* 10 4933.0*/38.000,	/* 11 5346.0*/35.000,	/* 12 5760.0*/40.000,	/* 13 6173.0*/41.000,	/* 14 6586.0*/30.750,	/* 15 7000.0*/40.000,	},
{/* 12 80.000	*//* 0 800.0*/12.250,	/* 1 1213.0*/7.750,	/* 2 1626.0*/16.750,	/* 3 2040.0*/18.000,	/* 4 2453.0*/25.250,	/* 5 2866.0*/25.667,	/* 6 3280.0*/27.500,	/* 7 3693.0*/28.500,	/* 8 4106.0*/24.500,	/* 9 4520.0*/25.500,	/* 10 4933.0*/36.000,	/* 11 5346.0*/32.000,	/* 12 5760.0*/37.000,	/* 13 6173.0*/38.000,	/* 14 6586.0*/37.750,	/* 15 7000.0*/40.000,	},
{/* 13 86.667	*//* 0 800.0*/4.000,	/* 1 1213.0*/-2.750,	/* 2 1626.0*/13.833,	/* 3 2040.0*/7.500,	/* 4 2453.0*/21.000,	/* 5 2866.0*/25.000,	/* 6 3280.0*/27.500,	/* 7 3693.0*/26.000,	/* 8 4106.0*/23.000,	/* 9 4520.0*/23.000,	/* 10 4933.0*/30.000,	/* 11 5346.0*/30.000,	/* 12 5760.0*/36.000,	/* 13 6173.0*/33.000,	/* 14 6586.0*/33.000,	/* 15 7000.0*/33.000,	},
{/* 14 93.333	*//* 0 800.0*/-7.500,	/* 1 1213.0*/-1.167,	/* 2 1626.0*/7.000,	/* 3 2040.0*/11.750,	/* 4 2453.0*/31.000,	/* 5 2866.0*/18.167,	/* 6 3280.0*/23.167,	/* 7 3693.0*/23.500,	/* 8 4106.0*/20.000,	/* 9 4520.0*/25.000,	/* 10 4933.0*/25.667,	/* 11 5346.0*/28.000,	/* 12 5760.0*/30.000,	/* 13 6173.0*/30.500,	/* 14 6586.0*/33.000,	/* 15 7000.0*/33.000,	},
{/* 15 100.000	*//* 0 800.0*/-5.714,	/* 1 1213.0*/-2.694,	/* 2 1626.0*/1.818,	/* 3 2040.0*/4.891,	/* 4 2453.0*/9.543,	/* 5 2866.0*/12.825,	/* 6 3280.0*/17.861,	/* 7 3693.0*/19.500,	/* 8 4106.0*/17.833,	/* 9 4520.0*/17.857,	/* 10 4933.0*/20.125,	/* 11 5346.0*/21.000,	/* 12 5760.0*/24.125,	/* 13 6173.0*/31.000,	/* 14 6586.0*/28.500,	/* 15 7000.0*/33.000	}
};
#endif


// http://rusefi.com/forum/viewtopic.php?f=3&t=360&start=40
//static float dodge_map_advance_table[16][16] = {
//{/*0 engineLoad=1.2*//*0 800.0*/-4.498, /*1 1213.0*/-11.905, /*2 1626.0*/-23.418, /*3 2040.0*/-25.357, /*4 2453.0*/-25.441, /*5 2866.0*/-25.468, /*6 3280.0*/-29.425, /*7 3693.0*/-32.713, /*8 4106.0*/-35.556, /*9 4520.0*/-37.594, /*10 4933.0*/-36.165, /*11 5346.0*/-30.578, /*12 5760.0*/-29.145, /*13 6173.0*/-29.065, /*14 6586.0*/-27.071, /*15 7000.0*/-28.282},
//{/*1 engineLoad=1.413333*//*0 800.0*/-4.87, /*1 1213.0*/-12.138, /*2 1626.0*/-23.389, /*3 2040.0*/-25.501, /*4 2453.0*/-25.441, /*5 2866.0*/-25.468, /*6 3280.0*/-29.125, /*7 3693.0*/-33.074, /*8 4106.0*/-34.203, /*9 4520.0*/-37.769, /*10 4933.0*/-35.899, /*11 5346.0*/-30.519, /*12 5760.0*/-28.88, /*13 6173.0*/-28.74, /*14 6586.0*/-27.189, /*15 7000.0*/-27.826},
//{/*2 engineLoad=1.626666*//*0 800.0*/-4.817, /*1 1213.0*/-12.262, /*2 1626.0*/-23.925, /*3 2040.0*/-25.501, /*4 2453.0*/-25.5, /*5 2866.0*/-25.468, /*6 3280.0*/-29.364, /*7 3693.0*/-33.489, /*8 4106.0*/-34.839, /*9 4520.0*/-37.545, /*10 4933.0*/-35.875, /*11 5346.0*/-30.353, /*12 5760.0*/-29.052, /*13 6173.0*/-28.37, /*14 6586.0*/-27.072, /*15 7000.0*/-26.828},
//{/*3 engineLoad=1.839999*//*0 800.0*/-4.537, /*1 1213.0*/-12.421, /*2 1626.0*/-23.214, /*3 2040.0*/-25.394, /*4 2453.0*/-25.412, /*5 2866.0*/-25.485, /*6 3280.0*/-29.425, /*7 3693.0*/-33.427, /*8 4106.0*/-34.091, /*9 4520.0*/-36.887, /*10 4933.0*/-36.047, /*11 5346.0*/-30.079, /*12 5760.0*/-28.453, /*13 6173.0*/-28.074, /*14 6586.0*/-27.189, /*15 7000.0*/-26.641},
//{/*4 engineLoad=2.053332*//*0 800.0*/-4.522, /*1 1213.0*/-11.76, /*2 1626.0*/-23.915, /*3 2040.0*/-25.415, /*4 2453.0*/-25.551, /*5 2866.0*/-25.14, /*6 3280.0*/-29.346, /*7 3693.0*/-32.917, /*8 4106.0*/-34.815, /*9 4520.0*/-37.211, /*10 4933.0*/-35.817, /*11 5346.0*/-29.694, /*12 5760.0*/-28.799, /*13 6173.0*/-27.818, /*14 6586.0*/-28.098, /*15 7000.0*/-27.662},
//{/*5 engineLoad=2.266665*//*0 800.0*/-4.678, /*1 1213.0*/-11.912, /*2 1626.0*/-23.486, /*3 2040.0*/-25.379, /*4 2453.0*/-25.551, /*5 2866.0*/-25.527, /*6 3280.0*/-29.856, /*7 3693.0*/-33.511, /*8 4106.0*/-34.786, /*9 4520.0*/-37.963, /*10 4933.0*/-35.917, /*11 5346.0*/-31.073, /*12 5760.0*/-28.361, /*13 6173.0*/-28.468, /*14 6586.0*/-27.188, /*15 7000.0*/-26.729},
//{/*6 engineLoad=2.479998*//*0 800.0*/-4.517, /*1 1213.0*/-12.029, /*2 1626.0*/-23.477, /*3 2040.0*/-25.455, /*4 2453.0*/-25.382, /*5 2866.0*/-25.898, /*6 3280.0*/-29.147, /*7 3693.0*/-33.578, /*8 4106.0*/-34.12, /*9 4520.0*/-36.279, /*10 4933.0*/-36.432, /*11 5346.0*/-31.362, /*12 5760.0*/-28.084, /*13 6173.0*/-28.463, /*14 6586.0*/-27.691, /*15 7000.0*/-27.83},
//{/*7 engineLoad=2.693331*//*0 800.0*/-4.532, /*1 1213.0*/-12.262, /*2 1626.0*/-23.935, /*3 2040.0*/-25.489, /*4 2453.0*/-25.595, /*5 2866.0*/-26.816, /*6 3280.0*/-30.251, /*7 3693.0*/-33.533, /*8 4106.0*/-34.794, /*9 4520.0*/-37.882, /*10 4933.0*/-36.104, /*11 5346.0*/-30.079, /*12 5760.0*/-28.545, /*13 6173.0*/-29.304, /*14 6586.0*/-27.07, /*15 7000.0*/-28.324},
//{/*8 engineLoad=2.906664*//*0 800.0*/-4.532, /*1 1213.0*/-12.036, /*2 1626.0*/-23.418, /*3 2040.0*/-25.513, /*4 2453.0*/-25.382, /*5 2866.0*/-25.357, /*6 3280.0*/-29.934, /*7 3693.0*/-33.467, /*8 4106.0*/-34.748, /*9 4520.0*/-37.288, /*10 4933.0*/-36.38, /*11 5346.0*/-29.516, /*12 5760.0*/-28.799, /*13 6173.0*/-28.407, /*14 6586.0*/-26.951, /*15 7000.0*/-28.203},
//{/*9 engineLoad=3.119997*//*0 800.0*/-4.532, /*1 1213.0*/-11.978, /*2 1626.0*/-23.73, /*3 2040.0*/-25.501, /*4 2453.0*/-25.624, /*5 2866.0*/-26.328, /*6 3280.0*/-30.015, /*7 3693.0*/-33.187, /*8 4106.0*/-34.881, /*9 4520.0*/-38.044, /*10 4933.0*/-35.81, /*11 5346.0*/-29.843, /*12 5760.0*/-29.306, /*13 6173.0*/-28.997, /*14 6586.0*/-27.109, /*15 7000.0*/-29.339},
//{/*10 engineLoad=3.33333*//*0 800.0*/-4.527, /*1 1213.0*/-12.131, /*2 1626.0*/-23.486, /*3 2040.0*/-25.43, /*4 2453.0*/-25.551, /*5 2866.0*/-26.276, /*6 3280.0*/-29.639, /*7 3693.0*/-33.005, /*8 4106.0*/-34.253, /*9 4520.0*/-37.788, /*10 4933.0*/-36.077, /*11 5346.0*/-30.188, /*12 5760.0*/-29.087, /*13 6173.0*/-28.481, /*14 6586.0*/-27.348, /*15 7000.0*/-27.777},
//{/*11 engineLoad=3.546663*//*0 800.0*/-4.889, /*1 1213.0*/-12.175, /*2 1626.0*/-23.271, /*3 2040.0*/-25.357, /*4 2453.0*/-25.551, /*5 2866.0*/-25.485, /*6 3280.0*/-29.899, /*7 3693.0*/-32.802, /*8 4106.0*/-34.786, /*9 4520.0*/-38.686, /*10 4933.0*/-35.722, /*11 5346.0*/-31.347, /*12 5760.0*/-28.891, /*13 6173.0*/-28.333, /*14 6586.0*/-27.149, /*15 7000.0*/-27.236},
//{/*12 engineLoad=3.759996*//*0 800.0*/-4.537, /*1 1213.0*/-12.073, /*2 1626.0*/-23.896, /*3 2040.0*/-25.525, /*4 2453.0*/-25.595, /*5 2866.0*/-25.451, /*6 3280.0*/-30.428, /*7 3693.0*/-33.714, /*8 4106.0*/-34.08, /*9 4520.0*/-37.526, /*10 4933.0*/-35.817, /*11 5346.0*/-30.733, /*12 5760.0*/-28.718, /*13 6173.0*/-28.518, /*14 6586.0*/-27.518, /*15 7000.0*/-26.561},
//{/*13 engineLoad=3.973329*//*0 800.0*/-4.86, /*1 1213.0*/-11.883, /*2 1626.0*/-23.428, /*3 2040.0*/-25.489, /*4 2453.0*/-25.536, /*5 2866.0*/-25.613, /*6 3280.0*/-29.895, /*7 3693.0*/-33.648, /*8 4106.0*/-34.758, /*9 4520.0*/-37.988, /*10 4933.0*/-36.047, /*11 5346.0*/-30.225, /*12 5760.0*/-28.698, /*13 6173.0*/-28.487, /*14 6586.0*/-27.111, /*15 7000.0*/-27.708},
//{/*14 engineLoad=4.186662*//*0 800.0*/-4.683, /*1 1213.0*/-11.898, /*2 1626.0*/-23.506, /*3 2040.0*/-25.562, /*4 2453.0*/-25.61, /*5 2866.0*/-25.519, /*6 3280.0*/-29.95, /*7 3693.0*/-33.582, /*8 4106.0*/-34.548, /*9 4520.0*/-36.201, /*10 4933.0*/-35.788, /*11 5346.0*/-30.053, /*12 5760.0*/-28.292, /*13 6173.0*/-28.259, /*14 6586.0*/-27.269, /*15 7000.0*/-26.863},
//{/*15 engineLoad=4.3999950000000005*//*0 800.0*/-4.85, /*1 1213.0*/-12.24, /*2 1626.0*/-24.091, /*3 2040.0*/-25.394, /*4 2453.0*/-25.323, /*5 2866.0*/-25.544, /*6 3280.0*/-29.915, /*7 3693.0*/-33.104, /*8 4106.0*/-36.016, /*9 4520.0*/-37.933, /*10 4933.0*/-36.254, /*11 5346.0*/-29.712, /*12 5760.0*/-28.651, /*13 6173.0*/-28.045, /*14 6586.0*/-27.228, /*15 7000.0*/-27.784}
//};

static const fuel_table_t veDodgeNeon2003Table = {
		/* Generated by TS2C on Sat Feb 27 12:27:00 EST 2016*/
		{/* 0 20.000	*//* 0 650.0*/50.000,	/* 1 800.0*/50.000,	/* 2 1050.0*/45.711,	/* 3 1300.0*/37.745,	/* 4 1550.0*/35.271,	/* 5 1800.0*/36.824,	/* 6 2050.0*/38.181,	/* 7 2300.0*/38.348,	/* 8 2550.0*/38.456,	/* 9 2800.0*/37.169,	/* 10 3050.0*/40.770,	/* 11 3300.0*/44.283,	/* 12 3550.0*/45.523,	/* 13 3800.0*/49.706,	/* 14 4050.0*/58.258,	/* 15 7000.0*/50.124,	},
		{/* 1 26.000	*//* 0 650.0*/48.551,	/* 1 800.0*/43.180,	/* 2 1050.0*/43.855,	/* 3 1300.0*/41.292,	/* 4 1550.0*/37.709,	/* 5 1800.0*/40.769,	/* 6 2050.0*/46.820,	/* 7 2300.0*/49.151,	/* 8 2550.0*/44.705,	/* 9 2800.0*/38.873,	/* 10 3050.0*/40.612,	/* 11 3300.0*/47.016,	/* 12 3550.0*/53.681,	/* 13 3800.0*/56.231,	/* 14 4050.0*/58.261,	/* 15 7000.0*/56.832,	},
		{/* 2 33.000	*//* 0 650.0*/47.201,	/* 1 800.0*/43.725,	/* 2 1050.0*/43.626,	/* 3 1300.0*/42.762,	/* 4 1550.0*/46.438,	/* 5 1800.0*/49.134,	/* 6 2050.0*/50.562,	/* 7 2300.0*/52.906,	/* 8 2550.0*/50.900,	/* 9 2800.0*/47.589,	/* 10 3050.0*/50.796,	/* 11 3300.0*/56.770,	/* 12 3550.0*/63.191,	/* 13 3800.0*/63.661,	/* 14 4050.0*/62.993,	/* 15 7000.0*/62.514,	},
		{/* 3 40.000	*//* 0 650.0*/50.000,	/* 1 800.0*/50.929,	/* 2 1050.0*/57.976,	/* 3 1300.0*/57.464,	/* 4 1550.0*/56.629,	/* 5 1800.0*/58.909,	/* 6 2050.0*/59.538,	/* 7 2300.0*/60.754,	/* 8 2550.0*/56.594,	/* 9 2800.0*/55.017,	/* 10 3050.0*/58.128,	/* 11 3300.0*/64.483,	/* 12 3550.0*/68.120,	/* 13 3800.0*/67.438,	/* 14 4050.0*/67.265,	/* 15 7000.0*/64.521,	},
		{/* 4 46.000	*//* 0 650.0*/50.626,	/* 1 800.0*/57.269,	/* 2 1050.0*/61.330,	/* 3 1300.0*/63.462,	/* 4 1550.0*/63.386,	/* 5 1800.0*/60.610,	/* 6 2050.0*/59.559,	/* 7 2300.0*/60.810,	/* 8 2550.0*/61.005,	/* 9 2800.0*/60.509,	/* 10 3050.0*/63.020,	/* 11 3300.0*/66.838,	/* 12 3550.0*/70.760,	/* 13 3800.0*/69.104,	/* 14 4050.0*/67.212,	/* 15 7000.0*/63.882,	},
		{/* 5 53.000	*//* 0 650.0*/52.141,	/* 1 800.0*/63.334,	/* 2 1050.0*/68.863,	/* 3 1300.0*/65.282,	/* 4 1550.0*/67.854,	/* 5 1800.0*/66.751,	/* 6 2050.0*/64.979,	/* 7 2300.0*/65.521,	/* 8 2550.0*/64.146,	/* 9 2800.0*/65.208,	/* 10 3050.0*/65.548,	/* 11 3300.0*/68.049,	/* 12 3550.0*/71.905,	/* 13 3800.0*/70.726,	/* 14 4050.0*/71.159,	/* 15 7000.0*/62.932,	},
		{/* 6 60.000	*//* 0 650.0*/50.510,	/* 1 800.0*/58.092,	/* 2 1050.0*/62.755,	/* 3 1300.0*/65.530,	/* 4 1550.0*/67.684,	/* 5 1800.0*/65.372,	/* 6 2050.0*/68.865,	/* 7 2300.0*/69.934,	/* 8 2550.0*/66.011,	/* 9 2800.0*/65.027,	/* 10 3050.0*/68.688,	/* 11 3300.0*/68.285,	/* 12 3550.0*/65.666,	/* 13 3800.0*/57.860,	/* 14 4050.0*/73.116,	/* 15 7000.0*/64.326,	},
		{/* 7 66.000	*//* 0 650.0*/50.000,	/* 1 800.0*/53.137,	/* 2 1050.0*/58.515,	/* 3 1300.0*/67.194,	/* 4 1550.0*/69.333,	/* 5 1800.0*/69.506,	/* 6 2050.0*/66.414,	/* 7 2300.0*/69.887,	/* 8 2550.0*/69.344,	/* 9 2800.0*/65.808,	/* 10 3050.0*/63.449,	/* 11 3300.0*/63.330,	/* 12 3550.0*/66.391,	/* 13 3800.0*/67.937,	/* 14 4050.0*/75.347,	/* 15 7000.0*/66.520,	},
		{/* 8 73.000	*//* 0 650.0*/50.000,	/* 1 800.0*/52.479,	/* 2 1050.0*/59.415,	/* 3 1300.0*/69.136,	/* 4 1550.0*/71.048,	/* 5 1800.0*/69.166,	/* 6 2050.0*/68.745,	/* 7 2300.0*/69.563,	/* 8 2550.0*/70.840,	/* 9 2800.0*/65.340,	/* 10 3050.0*/64.019,	/* 11 3300.0*/65.663,	/* 12 3550.0*/72.089,	/* 13 3800.0*/71.710,	/* 14 4050.0*/71.818,	/* 15 7000.0*/63.328,	},
		{/* 9 80.000	*//* 0 650.0*/50.000,	/* 1 800.0*/55.606,	/* 2 1050.0*/56.680,	/* 3 1300.0*/64.895,	/* 4 1550.0*/69.790,	/* 5 1800.0*/71.390,	/* 6 2050.0*/66.446,	/* 7 2300.0*/67.940,	/* 8 2550.0*/69.405,	/* 9 2800.0*/67.392,	/* 10 3050.0*/63.423,	/* 11 3300.0*/65.188,	/* 12 3550.0*/66.690,	/* 13 3800.0*/65.273,	/* 14 4050.0*/67.290,	/* 15 7000.0*/62.481,	},
		{/* 10 86.000	*//* 0 650.0*/50.819,	/* 1 800.0*/55.782,	/* 2 1050.0*/57.472,	/* 3 1300.0*/63.877,	/* 4 1550.0*/75.693,	/* 5 1800.0*/65.295,	/* 6 2050.0*/64.606,	/* 7 2300.0*/70.612,	/* 8 2550.0*/66.363,	/* 9 2800.0*/59.857,	/* 10 3050.0*/62.272,	/* 11 3300.0*/69.232,	/* 12 3550.0*/66.103,	/* 13 3800.0*/53.707,	/* 14 4050.0*/68.954,	/* 15 7000.0*/68.211,	},
		{/* 11 93.000	*//* 0 650.0*/50.893,	/* 1 800.0*/60.504,	/* 2 1050.0*/58.586,	/* 3 1300.0*/60.000,	/* 4 1550.0*/59.586,	/* 5 1800.0*/61.823,	/* 6 2050.0*/68.204,	/* 7 2300.0*/62.324,	/* 8 2550.0*/61.987,	/* 9 2800.0*/61.033,	/* 10 3050.0*/61.857,	/* 11 3300.0*/63.566,	/* 12 3550.0*/68.671,	/* 13 3800.0*/64.946,	/* 14 4050.0*/76.452,	/* 15 7000.0*/72.967,	},
		{/* 12 100.000	*//* 0 650.0*/50.000,	/* 1 800.0*/57.131,	/* 2 1050.0*/55.322,	/* 3 1300.0*/55.398,	/* 4 1550.0*/52.569,	/* 5 1800.0*/51.964,	/* 6 2050.0*/52.396,	/* 7 2300.0*/51.056,	/* 8 2550.0*/50.000,	/* 9 2800.0*/50.000,	/* 10 3050.0*/50.000,	/* 11 3300.0*/57.607,	/* 12 3550.0*/60.201,	/* 13 3800.0*/67.047,	/* 14 4050.0*/80.092,	/* 15 7000.0*/70.286,	},
		{/* 13 106.000	*//* 0 650.0*/50.000,	/* 1 800.0*/50.000,	/* 2 1050.0*/50.000,	/* 3 1300.0*/50.000,	/* 4 1550.0*/50.000,	/* 5 1800.0*/50.000,	/* 6 2050.0*/50.000,	/* 7 2300.0*/50.000,	/* 8 2550.0*/50.000,	/* 9 2800.0*/50.000,	/* 10 3050.0*/50.000,	/* 11 3300.0*/50.000,	/* 12 3550.0*/50.000,	/* 13 3800.0*/50.515,	/* 14 4050.0*/60.917,	/* 15 7000.0*/54.902,	},
		{/* 14 113.000	*//* 0 650.0*/50.000,	/* 1 800.0*/50.000,	/* 2 1050.0*/50.000,	/* 3 1300.0*/50.000,	/* 4 1550.0*/50.000,	/* 5 1800.0*/50.000,	/* 6 2050.0*/50.000,	/* 7 2300.0*/50.000,	/* 8 2550.0*/50.000,	/* 9 2800.0*/50.000,	/* 10 3050.0*/50.000,	/* 11 3300.0*/50.000,	/* 12 3550.0*/50.000,	/* 13 3800.0*/50.000,	/* 14 4050.0*/50.000,	/* 15 7000.0*/50.000,	},
		{/* 15 120.000	*//* 0 650.0*/50.000,	/* 1 800.0*/50.000,	/* 2 1050.0*/50.000,	/* 3 1300.0*/50.000,	/* 4 1550.0*/50.000,	/* 5 1800.0*/50.000,	/* 6 2050.0*/50.000,	/* 7 2300.0*/50.000,	/* 8 2550.0*/50.000,	/* 9 2800.0*/50.000,	/* 10 3050.0*/50.000,	/* 11 3300.0*/50.000,	/* 12 3550.0*/50.000,	/* 13 3800.0*/50.000,	/* 14 4050.0*/50.000,	/* 15 7000.0*/50.000,	},
};


EXTERN_CONFIG;

void setDodgeNeon1995EngineConfiguration(DECLARE_CONFIG_PARAMETER_SIGNATURE) {
	setDefaultFrankensoConfiguration(PASS_CONFIG_PARAMETER_SIGNATURE);

	engineConfiguration->trigger.type = TT_DODGE_NEON_1995;

	engineConfiguration->fuelAlgorithm = LM_ALPHA_N;

//	engineConfiguration->spi2SckMode = PAL_STM32_OTYPE_OPENDRAIN; // 4
//	engineConfiguration->spi2MosiMode = PAL_STM32_OTYPE_OPENDRAIN; // 4
//	engineConfiguration->spi2MisoMode = PAL_STM32_PUDR_PULLUP; // 32

	//	engineConfiguration->spi2mosiPin = GPIOB_15;
	//	engineConfiguration->spi2misoPin = GPIOB_14;
	//	engineConfiguration->spi2sckPin = GPIOB_13;
	engineConfiguration->cj125CsPin = GPIOB_0; // rev 0.4
	engineConfiguration->isCJ125Enabled = true;
	engineConfiguration->is_enabled_spi_2 = true;


	// set_rpm_hard_limit 4000
	engineConfiguration->rpmHardLimit = 4000; // yes, 4k. let's play it safe for now
	// set_cranking_rpm 550
	engineConfiguration->cranking.rpm = 550;


//	engineConfiguration->useOnlyRisingEdgeForTrigger = true;

	/**
	 * that's 1995 config
	 */

	setWholeTimingTable_d(12 PASS_CONFIG_PARAMETER_SUFFIX);

	// set cranking_injection_mode 0
	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	// set injection_mode 1
	engineConfiguration->injectionMode = IM_SEQUENTIAL;

	// this is needed for injector lag auto-tune research if switching to batch
	// enable two_wire_batch_injection
	engineConfiguration->twoWireBatchInjection = true;

	// set ignition_mode 2
	engineConfiguration->ignitionMode = IM_WASTED_SPARK;
	// set_firing_order 2
	engineConfiguration->specs.firingOrder = FO_1_3_4_2;

	// set global_trigger_offset_angle 497
	engineConfiguration->globalTriggerAngleOffset = 497;
	// set ignition_offset 350
	engineConfiguration->ignitionOffset = 350;
	 // set injection_offset 510
	engineConfiguration->extraInjectionOffset = 510 + 497;

	/**
	 * that's 1995 config
	 */

	// set cranking_charge_angle 70
	engineConfiguration->crankingChargeAngle = 70;
	// set cranking_timing_angle 0
	engineConfiguration->crankingTimingAngle = 0;

	// Frankenstein: low side - out #1: PC14
	// Frankenstein: low side - out #2: PC15
	// Frankenstein: low side - out #3: PE6
	// Frankenstein: low side - out #4: PC13
	// Frankenstein: low side - out #5: PE4
	// Frankenstein: low side - out #6: PE5
	// Frankenstein: low side - out #7: PE2
	// Frankenstein: low side - out #8: PE3
	// Frankenstein: low side - out #9: PE0
	// Frankenstein: low side - out #10: PE1
	// Frankenstein: low side - out #11: PB8
	// Frankenstein: low side - out #12: PB9

	engineConfiguration->injectionPins[0] = GPIOB_9; // Frankenstein: low side - out #12
	engineConfiguration->injectionPins[1] = GPIOB_8; // Frankenstein: low side - out #11
	engineConfiguration->injectionPins[2] = GPIOE_3; // Frankenstein: low side - out #8
	engineConfiguration->injectionPins[3] = GPIOE_5; // Frankenstein: low side - out #6

	engineConfiguration->fuelPumpPin = GPIOC_13; // Frankenstein: low side - out #4
	engineConfiguration->fuelPumpPinMode = OM_DEFAULT;

	engineConfiguration->mapErrorDetectionTooHigh = 120;

	// set injection_pin_mode 0
	engineConfiguration->injectionPinMode = OM_DEFAULT;

	// Frankenstein: high side #1: PE8
	// Frankenstein: high side #2: PE10

	engineConfiguration->ignitionPins[0] = GPIOE_8; // Frankenstein: high side #1
	engineConfiguration->ignitionPins[1] = GPIO_UNASSIGNED;
	engineConfiguration->ignitionPins[2] = GPIOE_10; // // Frankenstein: high side #2

	// set ignition_pin_mode 0
	engineConfiguration->ignitionPinMode = OM_DEFAULT;

	engineConfiguration->clt.config = {0, 30, 100, 32500, 7550, 700, 2700};

	engineConfiguration->sensorChartFrequency = 7;
}

void setDodgeNeonNGCEngineConfiguration(DECLARE_CONFIG_PARAMETER_SIGNATURE) {
	setDefaultFrankensoConfiguration(PASS_CONFIG_PARAMETER_SIGNATURE);
	engineConfiguration->trigger.type = TT_DODGE_NEON_2003_CAM;
	setFrankenso_01_LCD(engineConfiguration);
	setFrankenso0_1_joystick(engineConfiguration);

	// set global_trigger_offset_angle 38
	engineConfiguration->globalTriggerAngleOffset = 38;
	// set injection_offset 0
	engineConfiguration->extraInjectionOffset = 0;

	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_SEQUENTIAL;
	engineConfiguration->ignitionMode = IM_WASTED_SPARK;
	engineConfiguration->specs.displacement = 1.996;
	engineConfiguration->specs.cylindersCount = 4;

	/**
	 * 77C
	 * 1200 rpm
	 * fuel 3
	 *
	 * 88C
	 * fuel 2.8
	 *
	 */
	//setWholeTimingTable_d(12 PASS_CONFIG_PARAMETER_SUFFIX);
#if IGN_LOAD_COUNT == DEFAULT_IGN_LOAD_COUNT
	MEMCPY(config->ignitionTable, fromODB);
#endif

	copy2DTable<FSIO_TABLE_8, FSIO_TABLE_8, float, float>(vBattTarget, config->fsioTable1);

	MEMCPY(config->veTable, veDodgeNeon2003Table);
	//setMap(config->veTable, 50);

	// set cranking_charge_angle 70
	engineConfiguration->crankingChargeAngle = 70;
	// set cranking_timing_angle 710
	engineConfiguration->crankingTimingAngle = -710;

	/**
	 * bosch 4G1139
	 * http://forum.2gn.org/viewtopic.php?t=21657
	 * or is it 225 as mentioned at http://turbobazar.ru/showpost.php?p=750815&postcount=796 ?
	 */
	engineConfiguration->injector.flow = 199;

	setLinearCurve(config->ignitionLoadBins, 20, 120, 1);

	setAlgorithm(LM_SPEED_DENSITY PASS_CONFIG_PARAMETER_SUFFIX);

	setFuelTablesLoadBin(20, 120 PASS_CONFIG_PARAMETER_SUFFIX);

	engineConfiguration->malfunctionIndicatorPin = GPIO_UNASSIGNED;

	/**
	 * PA4 Wideband O2 Sensor
	 */
	engineConfiguration->afr.hwChannel = EFI_ADC_4;

	commonFrankensoAnalogInputs(engineConfiguration);
	engineConfiguration->vbattDividerCoeff = 9.75;// ((float) (8.2 + 33)) / 8.2 * 2;

	/**
	 * http://rusefi.com/wiki/index.php?title=Manual:Hardware_Frankenso_board
	 */
	// Frankenso low out #1: PE6 main relay
	// Frankenso low out #2: PE5
	// Frankenso low out #3: PD7 coolant fan relay
	// Frankenso low out #4: PC13 idle valve solenoid
	// Frankenso low out #5: PE3 fuel pump relay
	// Frankenso low out #6: PE4
	// Frankenso low out #7: PE1 (do not use with discovery!)
	// Frankenso low out #8: PE2 injector #3
	// Frankenso low out #9: PB9 injector #2
	// Frankenso low out #10: PE0 (do not use with discovery!)
	// Frankenso low out #11: PB8 injector #1
	// Frankenso low out #12: PB7 injector #4

	engineConfiguration->fanPin = GPIOD_7;

	engineConfiguration->injectionPins[0] = GPIOB_8;
	engineConfiguration->injectionPins[1] = GPIOB_9;
	engineConfiguration->injectionPins[2] = GPIOE_2;
	engineConfiguration->injectionPins[3] = GPIOB_7;

	engineConfiguration->ignitionPins[0] = GPIOC_9;
	engineConfiguration->ignitionPins[1] = GPIO_UNASSIGNED;
	engineConfiguration->ignitionPins[2] = GPIOE_8;
	engineConfiguration->ignitionPins[3] = GPIO_UNASSIGNED;

	engineConfiguration->mainRelayPin = GPIOE_6;

	engineConfiguration->idle.solenoidPin = GPIOC_13;
	engineConfiguration->idle.solenoidFrequency = 300;
	engineConfiguration->manIdlePosition = 36;

	engineConfiguration->fuelPumpPin = GPIOE_3;
	engineConfiguration->fuelPumpPinMode = OM_DEFAULT;

	engineConfiguration->triggerInputPins[0] = GPIOA_5;
	engineConfiguration->triggerInputPins[1] = GPIOC_6;

	/**
	 * Frankenso analog #1 PC2 ADC12 CLT
	 * Frankenso analog #2 PC1 ADC11 IAT
	 * Frankenso analog #3 PA0 ADC0 MAP
	 * Frankenso analog #4 PC3 ADC13
	 * Frankenso analog #5 PA2 ADC2 TPS
	 * Frankenso analog #6 PA1 ADC1
	 * Frankenso analog #7 PA4 ADC4 WBO AFR
	 * Frankenso analog #8 PA3 ADC3
	 * Frankenso analog #9 PA7 ADC7
	 * Frankenso analog #10 PA6 ADC6
	 * Frankenso analog #11 PC5 ADC15
	 * Frankenso analog #12 PC4 ADC14 VBatt
	 */


	setDodgeSensor(&engineConfiguration->clt, 10000);
	setDodgeSensor(&engineConfiguration->iat, 10000);

	/**
	 * MAP PA0
	 */
	engineConfiguration->map.sensor.hwChannel = EFI_ADC_0; // PA0

//	rev 0.1 green board
//	engineConfiguration->map.sensor.hwChannel = EFI_ADC_6; // PA6
//	engineConfiguration->tps1_1AdcChannel = EFI_ADC_15; // PC5


	/**
	 * TPS
	 */
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_2;
	engineConfiguration->tpsMax = 625; // convert 12to10 bit (ADC/4)
	engineConfiguration->tpsMin = 125; // convert 12to10 bit (ADC/4)

	/**
	 * IAT D15/W7
	 */
	engineConfiguration->iat.adcChannel = EFI_ADC_11;

	/**
	 * CLT D13/W9
	 */
	engineConfiguration->clt.adcChannel = EFI_ADC_12;

	engineConfiguration->sensorChartMode = SC_MAP;
	engineConfiguration->map.sensor.type = MT_DODGE_NEON_2003;

	engineConfiguration->hip9011Gain = 0.3;

	float t = 0.5;

	engineConfiguration->knockNoise[0] = 2.1 + t; // 800
	engineConfiguration->knockNoise[1] = 2.1 + t; // 1700
	engineConfiguration->knockNoise[2] = 2.2 + t; // 2600
	engineConfiguration->knockNoise[3] = 2.2 + t; // 3400
	engineConfiguration->knockNoise[4] = 2.3 + t; // 4300
	engineConfiguration->knockNoise[5] = 2.7 + t; // 5200
	engineConfiguration->knockNoise[6] = 3.1 + t; // 6100
	engineConfiguration->knockNoise[7] = 3.3 + t; // 7000


	engineConfiguration->cylinderBore = 87.5;

	engineConfiguration->clutchDownPin = GPIOC_12;
	engineConfiguration->clutchDownPinMode = PI_PULLUP;
//	engineConfiguration->clutchUpPin = GPIOA_14; // note SWCLK - conflict with SWD
	engineConfiguration->clutchUpPinMode = PI_PULLUP;

	engineConfiguration->auxPidPins[0] = GPIOD_5; // playing with AUX PID for alternator
	engineConfiguration->auxPidFrequency[0] = 300;

//	engineConfiguration->isVerboseAuxPid1 = true;
	engineConfiguration->auxPid[0].offset = 10;
	engineConfiguration->auxPid[0].pFactor = 5;
	engineConfiguration->auxPid[0].iFactor = 0.1;
	engineConfiguration->auxPid[0].dFactor = 0.1;

	
#if EFI_FSIO
//	/**
//	 * set_fsio_setting 1 0.55
//	 */
//	engineConfiguration->fsio_setting[0] = 0.55;
//	setFsioExt(0, GPIOE_5, "0 fsio_setting", 400 PASS_CONFIG_PARAMETER_SUFFIX);
#endif

	engineConfiguration->vehicleSpeedSensorInputPin = GPIOA_8;

	engineConfiguration->fanOnTemperature = 92;
	engineConfiguration->fanOffTemperature = 89;
//	engineConfiguration->fanOnTemperature = 115; // knock testing - value is a bit high
//	engineConfiguration->fanOffTemperature = 100;

//	engineConfiguration->tunerStudioSerialSpeed = 9600;
	engineConfiguration->tunerStudioSerialSpeed = 19200;
	setAlgorithm(LM_SPEED_DENSITY PASS_CONFIG_PARAMETER_SUFFIX);

//temp	engineConfiguration->alternatorControlPin = GPIOD_5;
	engineConfiguration->targetVBatt = 14.0;
	engineConfiguration->alternatorControl.offset = 20;
	engineConfiguration->alternatorControl.pFactor = 20;
	engineConfiguration->alternatorControl.iFactor = 0.2;
	engineConfiguration->alternatorControl.dFactor = 0.1;
	engineConfiguration->alternatorControl.periodMs = 10;

//	enableFrankensoCan();
	engineConfiguration->canWriteEnabled = true;
	engineConfiguration->canNbcType = CAN_BUS_NBC_BMW;
//	engineConfiguration->canNbcType = CAN_BUS_MAZDA_RX8;

	engineConfiguration->engineLoadAccelLength = 12;
	engineConfiguration->engineLoadAccelEnrichmentThreshold = 5; // kPa
	engineConfiguration->engineLoadAccelEnrichmentMultiplier = 0;

	engineConfiguration->tpsAccelLength = 12;
	engineConfiguration->tpsAccelEnrichmentThreshold = 10;

	engineConfiguration->wwaeTau = 1.0f;
	engineConfiguration->wwaeBeta = 0.40f;

	engineConfiguration->wwaeTau = 0;
	engineConfiguration->wwaeBeta = 0;

	engineConfiguration->isSdCardEnabled = false;
	engineConfiguration->manIdlePosition = 36; // set_idle_pwm 40

	engineConfiguration->slowAdcAlpha = 0.33333;

	engineConfiguration->isCylinderCleanupEnabled = true;

	// end of setDodgeNeonNGCEngineConfiguration
}
