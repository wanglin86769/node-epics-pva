const PV = require('node-epics-pva');

const start = Date.now();
let value;
for(let i = 1; i <= 500; i++) {
	value = PV.get(`calcExample${i}`);
}
const stop = Date.now();
console.log(`Time Taken to execute = ${1.0*(stop - start)/1000} seconds`);
