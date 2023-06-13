# EPICS pvAccess client for Node.js

**node-epics-pva** is lightweight EPICS pvAccess client library for Node.js, it is a FFI wrapper that talks to the PVXS shared library using a third-party Node.js FFI package called koffi.

Three simple interfaces are provided in the client library, get() and put() are simple blocking implementation,
 * get()
 * put()
 * monitor()

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

# Note

Since get() and put() are simple blocking implementation, if PVs are not available, the main thread of Node.js will block.

# License
MIT license