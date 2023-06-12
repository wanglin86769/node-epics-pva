const Channel = require('./channel');

const monitor = (pvname, callback) => {
    const pv = new Channel(pvname);
    pv.monitor();
    if(callback && typeof callback === 'function') {
        pv.on('value', callback);
    }
    return pv;
}

module.exports = monitor;
