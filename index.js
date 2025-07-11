import mqtt from 'mqtt';

const MQTT_BROKER_URL = 'mqtt://test.mosquitto.org';

const productMap = {
  "885370316087": { start: 0, end: 30 },
  "A-prgw69":      { start: 30, end: 60 },
  "5035223121800": { start: 60, end: 90 },
  "5026555260862": { start: 90, end: 120 }
};

const client = mqtt.connect(MQTT_BROKER_URL);

client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');

  client.subscribe('pbl/qr', (err) => {
    if (err) {
      console.error('❌ Failed to subscribe:', err.message);
    } else {
      console.log('📡 Subscribed to topic: pbl/qr');
    }
  });
});

client.on('message', (topic, message) => {
  if (topic === 'pbl/qr') {
    const qrCode = message.toString().trim();
    console.log(`📥 QR code received: "${qrCode}"`);

    const range = productMap[qrCode];
    if (range) {
      const payload = `${range.start}:${range.end}`;
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
