const Channel = require('./channel');

const get = (pvname) => {
    const pv = new Channel(pvname);
    const value = pv.get();
    return value;
}

module.exports = get;
