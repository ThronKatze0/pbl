import {connect} from 'mqtt';
import { randomBetween } from './coffee.js';

const options = {
    username: "m5stack",
    password: "c=Yu#DkuaL3}g^@"
}
const client = connect('mqtt://10.0.0.29', options);

const productMap = {
  "U162": { level: 1, start: 0, end: 10 },
  "U146": { level: 1, start: 10, end: 20 },
  "U122": { level: 1, start: 20, end: 30 },
  "U010": { level: 2, start: 0, end: 10 },
  "U175": { level: 2, start: 10, end: 20 },
  "U009": { level: 2, start: 20, end: 30 },
  "S001": "rainbow"
};

client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');

  client.subscribe('pbl/qr', (err) => {
    if (err) {
      console.error('❌ Failed to subscribe:', err.message);
    } else {
      console.log('📡 Subscribed to topic: pbl/qr');
    }
  });
  console.log("publish");
  client.publish('pbl/light', '1:0:10');
});

client.on('message', (topic, message) => {
  if (topic === 'pbl/qr') {
    const qrCode = message.toString().trim();
    console.log(`📥 QR code received: "${qrCode}"`);
    if (qrCode == "782004;200;mittel;S153002;") return;

    const range = productMap[qrCode];
    if (range) {
      let payload;
      if (range != "rainbow") {
        payload = `${range.level}:${range.start}:${range.end}`;
      } else {
        payload = "rainbow";
      }
      console.log(`💡 Sending to pbl/light → ${payload}`);
      client.publish('pbl/light', payload);
    } else {
      console.warn(`⚠️ Unknown QR code: "${qrCode}"`);
    }
  }
});

client.on('error', (err) => {
  console.error('❌ MQTT error:', err.message);
});
