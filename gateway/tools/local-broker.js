// Broker MQTT local para testes offline (equivalente a um Mosquitto local)
const { Aedes } = require('aedes');
(async () => {
  const aedes = await Aedes.createBroker();
  const server = require('net').createServer(aedes.handle);
  const port = Number(process.env.PORT || 1883);
  server.listen(port, () => console.log(`broker local em mqtt://localhost:${port}`));
})();
