const mqtt = require('mqtt');
const Influx = require("influx");

const mqttUsername = "admin";
const mqttPassword = "Taoualit2016$";

const influx = new Influx.InfluxDB({
  host: 'influxdb',
  port: 8086,
  database: 'iot-monitoring',
  username: 'root',
  password: 'root'
});

influx.getDatabaseNames()
  .then(names => {
    if (!names.includes('iot-monitoring')) {
      return influx.createDatabase('iot-monitoring');
    }
  });

let writeDataToInflux = (data) => {
  influx
      .writePoints(
          [
              {
                measurement: 'temperature',
                tags: {
                  deviceId: data.deviceId
                },
                fields: {
                  value: data.payload.temperature
                }
              }
           ]
      )
      .catch(error => {
        console.error("Error writing data to Influx - " + error);
      });
};

const main = async function () {
  console.log("MQTT Bridge started.");

  const topicSensorTempData = "/sensor/temp/data";
  const topicSensorTempStatus = "/sensor/temp/status";

  let mqttOptions = {
    username: mqttUsername,
    password: mqttPassword,
    clean:true
  };

  const mqttClient = mqtt.connect("mqtt://broker.emqx.io:1883", mqttOptions);
  console.log("connected flag = " + mqttClient.connected);

  mqttClient.subscribe(topicSensorTempData, {qos:1});
  mqttClient.subscribe(topicSensorTempStatus, {qos:1});

  // Handle incoming messages
  mqttClient.on('message', function(topic, message, packet) {
    console.log("Message arrived [" + topic + "] " + message);

    if (topic.toString().trim() === topicSensorTempData) {
      let data = {
        deviceId: "dht-sensor01",
        payload: JSON.parse(message.toString())
      };
      console.log(data);
      writeDataToInflux(data);
    }
  });

  mqttClient.on("connect", function() {
    console.log("connected = "+ mqttClient.connected);
  })
};

main().catch(function (error) {
  console.error("Error", error);
  process.exit(1);
});
