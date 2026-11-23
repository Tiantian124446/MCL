// ---------------------------------------------------------------------------
//  https_proxy.js  -  MCL 反向代理 / 端口转发
//    443 / 8443 (TLS)  : https://pocket4、https://<主机IP>、https://26.36.61.205 等
//    7891 / 8080 (HTTP): 由系统 netsh portproxy 转发到虚拟机（见 tools/server_watchdog.ps1）
//  监听本机所有局域网地址（WLAN / 校园网 / Radmin VPN 等），虚拟机 IP 变化会自动跟随。
// ---------------------------------------------------------------------------
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const os = require('os');

const BASE = 'E:/MCL/host';
const PFX = path.join(BASE, 'cert', 'mcl.pfx');
const PFX_PASS = process.env.MCL_CERT_PASS || 'MCLCert2026!';
const LOG = path.join(BASE, 'proxy.log');

// 后端地址: 优先 host/target.txt (看护任务写入), 每次请求重读(5 秒缓存)
let _targetCache = null;
let _targetCacheAt = 0;
function readTarget() {
  const now = Date.now();
  if (_targetCache && now - _targetCacheAt < 5000) return _targetCache;
  let t = null;
  try {
    const s = fs.readFileSync(path.join(BASE, 'target.txt'), 'utf8').trim();
    const m = /^([\d.]+):(\d+)$/.exec(s);
    if (m) t = { host: m[1], port: parseInt(m[2], 10) };
  } catch (e) {}
  if (!t) {
    t = {
      host: process.env.MCL_TARGET_HOST || '192.168.201.129',
      port: parseInt(process.env.MCL_TARGET_PORT || '7891', 10),
    };
  }
  _targetCache = t;
  _targetCacheAt = now;
  return t;
}

// 需要监听的地址: 本机所有非虚拟网卡的 IPv4（含 Radmin VPN，便于跨网络访问）
function detectBinds() {
  if (process.env.MCL_BIND) return [process.env.MCL_BIND];
  const skip = /vmnet|vmware|vethernet|wsl|loopback|teredo|isatap|本地连接\*|以太网 2/i;
  const nets = os.networkInterfaces();
  const out = [];
  for (const name of Object.keys(nets)) {
    if (skip.test(name)) continue;
    for (const ni of nets[name] || []) {
      if (ni.family === 'IPv4' && !ni.internal && !ni.address.startsWith('169.254.')) {
        if (!out.includes(ni.address)) out.push(ni.address);
      }
    }
  }
  if (!out.length) out.push('127.0.0.1');
  return out;
}

function log(msg) {
  const line = '[' + new Date().toISOString() + '] ' + msg + '\n';
  try { fs.appendFileSync(LOG, line); } catch (e) {}
  try { process.stdout.write(line); } catch (e) {}
}

function forward(req, res) {
  // 客户端证书下载（校园网机器一键信任用）
  if (req.url === '/mcl-ca.cer') {
    try {
      const cer = fs.readFileSync(path.join(BASE, 'cert', 'mcl.cer'));
      res.writeHead(200, {
        'content-type': 'application/x-x509-ca-cert',
        'content-disposition': 'attachment; filename="mcl-ca.cer"',
        'access-control-allow-origin': '*',
      });
      res.end(cer);
      return;
    } catch (e) {}
  }
  const target = readTarget();
  const headers = Object.assign({}, req.headers);
  headers['host'] = target.host + ':' + target.port;
  headers['x-forwarded-host'] = req.headers.host || '';
  headers['x-forwarded-proto'] = 'https';
  const opts = {
    host: target.host,
    port: target.port,
    method: req.method,
    path: req.url,
    headers: headers,
    timeout: 20000,
  };
  const p = http.request(opts, (up) => {
    res.writeHead(up.statusCode, up.headers);
    up.pipe(res);
  });
  p.on('timeout', () => { p.destroy(new Error('upstream timeout')); });
  p.on('error', (e) => {
    if (!res.headersSent) res.writeHead(502, { 'content-type': 'text/plain; charset=utf-8' });
    res.end('MCL proxy error: ' + e.message);
    log('upstream error ' + req.method + ' ' + req.url + ' -> ' + e.message);
  });
  req.pipe(p);
}

let tls;
try {
  tls = { pfx: fs.readFileSync(PFX), passphrase: PFX_PASS };
} catch (e) {
  log('cannot read pfx: ' + e.message);
  process.exit(1);
}

const BINDS = detectBinds();
log('binds = ' + BINDS.join(', '));

let okCount = 0;
for (const ip of BINDS) {
  // HTTPS: 443 + 8443（443 被 Steam++ 占用时，8443 可作校园网备用端口）
  for (const port of [443, 8443]) {
    const s = https.createServer(tls, forward);
    s.on('error', (e) => { log('https ' + port + '@' + ip + ' error: ' + (e.code || e.message)); });
    s.listen(port, ip, () => {
      okCount++;
      const t = readTarget();
      log('https ' + port + '@' + ip + ' -> http://' + t.host + ':' + t.port);
    });
  }
  // 7891 / 8080 由系统 netsh portproxy 转发（0.0.0.0，覆盖所有网卡），这里不再重复监听
}
log('listeners started: ' + okCount);
