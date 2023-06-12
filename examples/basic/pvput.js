const PV = require('node-epics-pva');

PV.put("calcExample", 10);
PV.put("rec:X", 40);
PV.put("rec:Y", { value: 50 });
PV.put("grp:name", { 'X.value': 10, 'Y.value': 20 });