# EPICS pvAccess client for Node.js

**node-epics-pva** is lightweight EPICS pvAccess client library for Node.js, it is a FFI wrapper that talks to the PVXS shared library using a third-party Node.js FFI package called koffi.

It provides three simple interfaces, in which get() and put() are simple blocking implementations,
 * get()
 * put()
 * monitor()

 The implementation of node-epics-pva is building a wrapper into the PVXS shared library to provide C interface and calling the C interface from Node.js using koffi. The format of data exchange between the wrapper and Node.js is JSON.

 ![Alt text](docs/screenshots/architecture.png?raw=true "Title")

# Requirements

## A recent Node.js version is required and the following versions have been tested,

* Node.js 14.21.3

* Node.js 15.14.0

* Node.js 16.19.0

* Node.js 17.9.1

* Node.js 18.13.0

# Supported platforms

* Windows x86_64
* Linux x86_64
* macOS x86_64

# Installation

```bash
npm install node-epics-pva
```

`NODE_EPICS_PVA_WAIT_TIMEOUT` can be set optionally, for example,

```bash
export NODE_EPICS_PVA_WAIT_TIMEOUT=2.0
```

# Usage

Simple functions like pvget, pvput and pvmonitor are provided.

### get

```javascript
const PV = require('node-epics-pva');

let value = PV.get('calcExample');
console.log(value);
```

### put

```javascript
const PV = require('node-epics-pva');

PV.put("calcExample", 10);
PV.put("rec:X", 40);
PV.put("rec:Y", { value: 50 });
PV.put("grp:name", { 'X.value': 10, 'Y.value': 20 });
```

### monitor

```javascript
const PV = require('node-epics-pva');

PV.monitor('calcExample', function(data) {
    console.log('Current:', data);
});

// Test purpose only, prevent the node.js main thread from exiting
setTimeout(function() {
    console.log("Done!!!");
}, 3600 * 1000);
```

# Example

```
record(calc, "calcExample")
{
        field(DESC, "Counter")
        field(SCAN,"1 second")
        field(FLNK, "aiExample")
        field(CALC, "(A<B)?(A+C):D")
        field(INPA, "calcExample.VAL  NPP NMS")
        field(INPB, "70")
        field(INPC, "1")
        field(INPD, "30")
        field(EGU, "Counts")
        field(HOPR, "10")
        field(HIHI, "8")
        field(HIGH, "6")
        field(LOW, "4")
        field(LOLO, "2")
        field(HHSV, "MAJOR")
        field(HSV, "MINOR")
        field(LSV, "MINOR")
        field(LLSV, "MAJOR")
}
record(waveform, "wf1Example")
{
        field(DTYP, "Soft Channel")
        field(FTVL, "DOUBLE")
        field(NELM, "16")
        field(INP, [1, 2, 3, 4, 5, 6, 7, 8])
}
```

Accessing the calc and waveform records above will return the following JSON data,

```json
{
	"id": "epics:nt/NTScalar:1.0",
	"value": 43,
	"alarm": {
		"id": "alarm_t",
		"severity": 2,
		"status": 1,
		"message": "HIHI"
	},
	"timeStamp": {
		"secondsPastEpoch": 1686719557,
		"nulloseconds": 144609643,
		"userTag": 0
	},
	"display": {
		"limitLow": 0,
		"limitHigh": 10,
		"description": "Counter",
		"units": "Counts",
		"precision": 0,
		"form": {
			"id": "enum_t",
			"index": 0,
			"choices": ["Default", "String", "Binary", "Decimal", "Hex", "Exponential", "Engineering"]
		}
	},
	"control": {
		"id": "control_t",
		"limitLow": 0,
		"limitHigh": 10,
		"minStep": 0
	},
	"valueAlarm": {
		"id": "valueAlarm_t",
		"active": 0,
		"lowAlarmLimit": 2,
		"lowWarningLimit": 4,
		"highWarningLimit": 6,
		"highAlarmLimit": 8,
		"lowAlarmSeverity": 0,
		"lowWarningSeverity": 0,
		"highWarningSeverity": 0,
		"highAlarmSeverity": 0,
		"hysteresis": 0
	}
}

```

```json
{
	"id": "epics:nt/NTScalarArray:1.0",
	"value": [1, 2, 3, 4, 5, 6, 7, 8],
	"alarm": {
		"id": "alarm_t",
		"severity": 3,
		"status": 2,
		"message": "UDF"
	},
	"timeStamp": {
		"secondsPastEpoch": 631152000,
		"nulloseconds": 0,
		"userTag": 0
	},
	"display": {
		"limitLow": 0,
		"limitHigh": 0,
		"description": "",
		"units": "",
		"precision": 0,
		"form": {
			"id": "enum_t",
			"index": 0,
			"choices": ["Default", "String", "Binary", "Decimal", "Hex", "Exponential", "Engineering"]
		}
	},
	"control": {
		"id": "control_t",
		"limitLow": 0,
		"limitHigh": 0,
		"minStep": 0
	},
	"valueAlarm": {
		"id": "valueAlarm_t",
		"active": 0,
		"lowAlarmLimit": null,
		"lowWarningLimit": null,
		"highWarningLimit": null,
		"highAlarmLimit": null,
		"lowAlarmSeverity": 0,
		"lowWarningSeverity": 0,
		"highWarningSeverity": 0,
		"highAlarmSeverity": 0,
		"hysteresis": 0
	}
}

```

# Performance

## Performance test for pvget and pvput,

| Number of PVs | pvget (seconds) | pvput (seconds) |
|------|-------------|-------------|
| 1 | 0.034 | 0.026 |
| 10 | 0.217 | 0.210 |
| 50 | 1.049 | 1.060 |
| 100 | 2.120 | 2.100 |
| 200 | 4.222 | 4.127 |
| 500 | 10.569 | 10.465 |

The test environment is as follows, the client and IOC are in different virtual machines and different subsets.

* Debian Linux 10 (buster)

* Intel Core Processor (Haswell), 4-core, 2.4GHz

* 8GB memory

# Note

1. get() and put() are simple blocking implementation, if PVs are not available, the main thread of Node.js will block.
2. Only epics:nt/NTScalar:1.0 and epics:nt/NTScalarArray:1.0 data types are tested, other data types are not guaranteed to be parse successfully.

# License
MIT license