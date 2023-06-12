const PV = require('node-epics-pva');

let value = PV.get('calcExample');
console.log(value);
   