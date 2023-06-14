const PV = require('node-epics-pva');

const start = Date.now();
for(let i = 1; i <= 500; i++) {
	PV.put(`calcExample${i}`, 10);
}
const stop = Date.now();
console.log(`Time Taken to execute = ${1.0*(stop - start)/1000} seconds`);