const koffi = require('koffi');
const EventEmitter = require('events');

//const LIBCA_PATH = '/home/debian/epics/pvxs-1.2.0/lib/linux-x86_64/libpvxs.so';
const LIBCA_PATH = 'D:\\Development2\\epics\\pvxs-1.2.0_wrapper\\bin\\windows-x64\\pvxs.dll';
const libca = koffi.load(LIBCA_PATH);

const pointer = koffi.pointer('pointer', koffi.opaque(), 2);

const MonitorCallback = koffi.proto('MonitorCallback', 'void', ['const char *']);

const simpleget = libca.func('simpleget', 'int', ['char *', koffi.out(pointer), 'double']);
const simpleput = libca.func('simpleput', 'int', ['char *', 'char **', 'char **', 'int', 'double']);
const simplemonitor = libca.func('simplemonitor', 'int', ['char *', koffi.pointer(MonitorCallback)]);

const WAIT_TIMEOUT = Number(process.env.NODE_EPICS_PVA_WAIT_TIMEOUT) || 5.0;

function postProcess(str) {
	// JSON syntax does not support NaN or Infinit, so replace "nan" with "null"
	str = str.replace(/nan/g, "null");

	// remove all trailing commas
	let regex = /\,(?=\s*?[\}\]])/g;
	str = str.replace(regex, '');
	return str;
}

function isObject(data) {
	return typeof data === 'object' && data !== null && !Array.isArray(data)
}

class Channel extends EventEmitter {
    pvname = null;
    monitorCallbackPtr = null;

    constructor(pvname) {
        super();
        this.pvname = pvname;
    }

    get() {
        let valuePtr = [null];
		let ret = simpleget(this.pvname, valuePtr, WAIT_TIMEOUT);
		if(ret == -1) return null;

		let str = koffi.decode(valuePtr[0], 'char', 10000);
		str = postProcess(str);
		let value = JSON.parse(str);
		return value;
    }

	put(data) {
		if(data === null || data === undefined) return -1;
		let fields = [];
		let values = [];
		let size = 0;
		if(isObject(data)) {
			for(const [key, value] of Object.entries(data)) {
				fields.push(key);
				values.push(String(value));
			}
			size = fields.length;
		} else {
			fields = ['value'];
			values = [String(data)];
			size = 1;
		}
        let ret = simpleput(this.pvname, fields, values, size, WAIT_TIMEOUT);
		return ret;
    }

    monitor() {
		this.monitorCallbackPtr = koffi.register(args => {
			let str = postProcess(args);
			let value = JSON.parse(str);
			this.emit('value', value);
		}, koffi.pointer(MonitorCallback));

		simplemonitor(this.pvname, this.monitorCallbackPtr);
    }

}

module.exports = Channel;