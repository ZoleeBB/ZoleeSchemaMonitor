import test from 'node:test';
import assert from 'node:assert/strict';
import {valueText} from '../web/model.js';

test('value formatting handles ties, carry, negative zero, units and unknown values consistently', () => {
    for (const [value, decimals, expected] of [
        [2.5,0,'3'],[-2.5,0,'-3'],[2.55,1,'2.6'],[-2.55,1,'-2.6'],
        [9.999,2,'10.00'],[-.0001,2,'0.00'],[-0,1,'0.0'],[0,6,'0.000000'],
        [24.5,1,'24.5'],[1e9,2,'1e+09'],[-1e12,0,'-1e+12'],[1.234567e10,3,'1.23457e+10'],
        [9.999999e9,6,'1e+10'],[Number.MAX_VALUE,6,'1.79769e+308'],[1e-300,6,'0.000000'],
    ]) {
        assert.equal(valueText({valid:true,value},decimals),expected);
        assert.equal(valueText({valid:true,value},decimals,'°C'),expected+' °C');
    }
    for (const signal of [{valid:false,value:1},{valid:true,value:NaN},{valid:true,value:Infinity}]) {
        assert.equal(valueText(signal,1,'°C'),'— °C');
        assert.equal(valueText(signal,1),'—');
    }
    for (const decimals of [-1,7,.5,NaN]) assert.throws(()=>valueText({valid:true,value:1},decimals));
});
