// ---------------------------------------------------------------------------
//  https_proxy.js  -  MCL 反向代理
//    https://pocket4-MCL.com      (443)  -> Win10 虚拟机 MCLServer (默认 192.168.201.129:7891)
//    http://192.168.3.173:7891    (7891) -> 同一后端（局域网回退地址）
//    http://<host>:80             -> 301 跳转到 https
//  使用自签证书 (host/cert/mcl.pfx)；只监听指定网卡，避免和占用 0.0.0.0:443 的其它程序冲突。
// ---------------------------------------------------------------------------
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

const BASE = 'E:/MCL/host';
const PFX = path.join(BASE, 'cert', 'mcl.pfx');
const PFX_PASS = process.env.MCL_CERT_PASS || 'MCLCert2026!';

// 后端地址: 优先 host/target.txt (看护任务会写入), 否则用环境变量/默认值
// 每次请求都会重新读取(带 5 秒缓存), 虚拟机 IP 变化后无需重启代理
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
// 默认监听本机局域网地址（0.0.0.0:443 常被 Steam++ 等程序占用）
function detectBind() {
  if (process.env.MCL_BIND) return process.env.MCL_BIND;
  const os = require('os');
  const nets = os.networkInterfaces();
  const skip = /vmnet|vmware|radmin|vethernet|wsl|loopback|本地连接\*|以太网 2/i;
  for (const name of Object.keys(nets)) {
    if (skip.test(name)) continue;
    for (const ni of nets[name] || []) {
      if (ni.family === 'IPv4' && !ni.internal) return ni.address;
    }
  }
  return '127.0.0.1';
}
const BIND = detectBind();
const LOG = path.join(BASE, 'proxy.log');

function log(msg) {
  const line = '[' + new Date().toISOString() + '] ' + msg + '\n';
  try { fs.appendFileSync(LOG, line); } catch (e) {}
  try { process.stdout.write(line); } catch (e) {}
}

function forward(req, res) {
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

const server = https.createServer(tls, forward);
server.on('error', (e) => { log('https listen error: ' + BIND + ':443 ' + e.message); });
server.listen(443, BIND, () => {
  const t = readTarget();
  log('https://pocket4:443 (' + BIND + ') -> http://' + t.host + ':' + t.port);
});

const lan = http.createServer(forward);
lan.on('error', (e) => { log('lan listen error: ' + BIND + ':7891 ' + e.message); });
lan.listen(7891, BIND, () => {
  const t = readTarget();
  log('http://' + BIND + ':7891 -> http://' + t.host + ':' + t.port + ' (LAN fallback)');
});

const redir = http.createServer((req, res) => {
  const host = (req.headers.host || 'pocket4').split(':')[0];
  res.writeHead(301, { location: 'https://' + host + req.url });
  res.end();
});
redir.on('error', (e) => { log('http:80 listen error (ignored): ' + e.message); });
redir.listen(80, BIND, () => { log('http://' + BIND + ':80 -> https redirect'); });
