const Channel = require('./channel');

const put = (pvname, data) => {
    const pv = new Channel(pvname);
    pv.put(data);
}

module.exports = put;
