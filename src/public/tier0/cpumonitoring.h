#ifndef CPU_MONITORING_H
#define CPU_MONITORING_H

/*
This header defines functions and structures for controlling the measurement of CPU frequency
in order to detect thermal throttling. For details see the associated source file.
*/

struct CPUFrequencyResults
{
	double m_timeStamp; // Time (from Plat_FloatTime) when the measurements were made.
	float m_GHz;
	float m_percentage;
	float m_lowestPercentage;
};

// Call this to get results.
// When CPU monitoring is 'disabled' it may still be running at a low frequency,
// for OGS purposes or for proactively warning users of problems. If fGetDisabledResults
// is true then results will be returned when disabled (if available).
PLATFORM_INTERFACE CPUFrequencyResults GetCPUFrequencyResults( bool fGetDisabledResults = false );

// Call this to set the monitoring frequency. Intervals of 2-5 seconds (2,000 to 5,000 ms)
// are recommended. An interval of zero will disable CPU monitoring. Short delays (below
// about 300 ms) will be rounded up.
PLATFORM_INTERFACE void SetCPUMonitoringInterval( unsigned nDelayMilliseconds );

// Warn with increasing strident colors when CPU percentages go below these levels.
// They are const int instead of float because const float in C++ is stupid.
const int kCPUMonitoringWarning1 = 80;
const int kCPUMonitoringWarning2 = 50;

#endif
