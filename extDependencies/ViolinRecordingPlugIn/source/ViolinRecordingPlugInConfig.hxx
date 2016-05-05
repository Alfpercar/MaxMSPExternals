#ifndef INCLUDED_VIOLINRECORDINGPLUGINCONFIG_HXX
#define INCLUDED_VIOLINRECORDINGPLUGINCONFIG_HXX

#define ENABLE_OPTICAL_SENSOR 1
#define MAX_NUM_VIOLINS 4

// Check if processBlock() is called periodically.
// This is mainly for modular hosts such as Bidule where a plug-in can exist before being 
// "connected", which means it shouldn't be allowed to connect, etc.
// If the tracker is connected and processing stops, it disconnects.
// Processing may also stop in non-modular hosts (say Cubase or Nuendo) when the plug-in is 
// explicitly turned off in the GUI (with the on/off button).
#define ENABLE_CHECK_PROCESSING_DEAD 1			// default = 1 (when breaking debugger in audio thread, use 0)

// Enable logging exact serial communication errors.
#define ENABLE_EXTENDED_SERIAL_COMM_ERROR_LOGGING 1

// For debugging purposes.
#define USE_ASYNCH_TRACKER_CONNECT_DISCONNECT 1	// default = 1

// Plot stick-bridge distance instead of sensor acceleration for visually checking synchronization.
#define PLOT_STICK_BRIDGE_DISTANCE 1

#define ALLOW_SYNC_CHANGES_WHILE_RECORDING 1	// default = 1

// For debugging purposes.
//#define USE_DUMMY_TRACKER_DATA 0	// default = 0

// For debugging purposes.
//#define USE_DUMMY_ARDUINO_DATA 0	// default = 0

// For debugging purposes.
#define DISABLE_TIMERS 0			// default = 0

// For debugging purposes.
#define DISABLE_SENDING_DATA 0		// default = 0

#endif