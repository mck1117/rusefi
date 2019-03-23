#pragma once

#include <cstdint>

enum class SensorType : uint8_t
{
    // Indicates disabled
    Disabled = 0,

    // Engine temperatures
    CoolantTemperature,
    IntakeTemperature,

    // Pressures
    ManifoldPressure1,
    ManifoldPressure2,
    BarometricPressure,

    // MAF
    MassAirFlow1,
    MassAirFlow2,

    // TPS outputs (two throttles)
    Tps1,
    Tps2,

    // TPS raw inputs (if you have redundant TPS sensors)
    TpsRaw1,
    TpsRaw2,
    TpsRaw3,
    TpsRaw4,

    // Lambda
    LambdaAverage,
    LambdaBank1,
    LambdaBank2,
    LambdaBank3,
    LambdaBank4,

    // Oil
    OilPressure,
    OilTemperature,
    OilLevel,

    // Fuel
    FuelLevel1,
    FuelLevel2,
    FuelPressure,
    FuelTemperature,

    // Knock
    KnockLevelBank1,
    KnockLevelBank2,

    // Battery voltage
    BatteryVoltage,

    // MCU Temp
    InternalTemperature,
    InternalTemperature2,

    // VSS
    VehicleSpeed,
    WheelSpeedFL,
    WheelSpeedFR,
    WheelSpeedRL,
    WheelSpeedRR,

    // Transmission
    TransmissionLinePressure,
    TransmissionRatio,
    TransmissionCurrentGear,
    TransmissionRequestedGear,
    TransmissionTemperature,

    // Auxiliary sensors
    Aux1,
    Aux2,
    Aux3,
    Aux4,
    Aux5,
    Aux6,
    Aux7,
    Aux8,

    DoNotUseLastSensor,
};
