// CLI de consulta ao Redis (Entrega 5)
// Uso:
//   node query.js devices
//   node query.js state    <deviceId>
//   node query.js history  <deviceId> [limite]
//   node query.js window   <deviceId> <minutos>     (leituras dos últimos N minutos)
//   node query.js commands <deviceId> [limite]
//   node query.js invalid  [limite]
//   node query.js valid    [limite]
const store = require('./store');

const fmtTime = (ms) => new Date(Number(ms)).toLocaleString('pt-BR', { timeZone: 'America/Sao_Paulo' });

async function main() {
  const [cmd, a, b] = process.argv.slice(2);
  await store.connect();
  try {
    switch (cmd) {
      case 'devices':
        console.log(await store.listDevices());
        break;
      case 'state': {
        const s = await store.getState(a);
        console.log(JSON.stringify(s, null, 2));
        break;
      }
      case 'history': {
        const rows = await store.getHistory(a, { limit: Number(b || 20) });
        console.log(`Total armazenado: ${await store.getHistoryCount(a)} | exibindo ${rows.length}`);
        console.table(rows.map((r) => ({
          recebido: fmtTime(r.receivedAt), seq: r.sequence, tempC: r.value, armed: r.armed, ledOn: r.ledOn, rssi: r.wifiRssi,
        })));
        break;
      }
      case 'window': {
        const to = Date.now();
        const rows = await store.getHistory(a, { fromMs: to - Number(b || 10) * 60000, toMs: to });
        const vals = rows.map((r) => r.value).filter((v) => v !== null);
        console.log(`${rows.length} leituras nos últimos ${b || 10} min` +
          (vals.length ? ` | min=${Math.min(...vals)} max=${Math.max(...vals)} média=${(vals.reduce((x, y) => x + y, 0) / vals.length).toFixed(2)} °C` : ''));
        break;
      }
      case 'commands':
        console.log(JSON.stringify(await store.getCommands(a, Number(b || 10)), null, 2));
        break;
      case 'invalid':
      case 'valid':
        console.log(JSON.stringify(await store.getLogs(cmd === 'valid', Number(a || 10)), null, 2));
        break;
      default:
        console.log('Comandos: devices | state <id> | history <id> [n] | window <id> <min> | commands <id> [n] | invalid [n] | valid [n]');
    }
  } finally {
    await store.close();
  }
}

main().catch((e) => { console.error(e.message); process.exit(1); });
