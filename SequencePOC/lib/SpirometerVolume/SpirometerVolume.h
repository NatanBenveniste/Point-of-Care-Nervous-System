#pragma once

// ============================================================
// SPIROMETER PUBLIC FUNCTIONS
// ============================================================
//
// This file is the public interface for the spirometer library.
//
// main.cpp should only call these functions.
// The pressure reading, filtering, breath detection, flow calculation,
// and volume averaging all happen inside SpirometerVolume.cpp.
// ============================================================

// Runs once in setup()
// Initializes SPI, starts the pressure sensor, sets sensor settings,
// and zeros the sensor to atmospheric pressure.
void spirometerBegin();

// Runs repeatedly in loop()
// Reads pressure, filters deltaP, detects breaths,
// integrates volume during each breath, and stores completed breaths.
void spirometerUpdate();

// Resets the 1-minute test data.
// This clears breath count, total volume, current breath volume,
// and filter state.
void spirometerResetTest();

// Returns the average completed breath volume in liters.
// average = total completed breath volume / number of completed breaths.
float spirometerGetAverageBreathVolume_L();

// Returns number of completed breaths detected during the test.
int spirometerGetBreathCount();

// Starts a new 1-minute spirometer test.
void spirometerStartTest();
// Returns true if the 1-minute test is done, false if still running.
bool spirometerRunTest();
