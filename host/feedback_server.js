// MCL feedback receiver on host. Saves BUG feedback to E:\MCL\feedback and records a pending queue.
const http = require('http');
const fs = require('fs');
const path = require('path');

const ROOT = 'E:\\MCL\\feedback';
const PORT = 8098;
const LOG = path.join(ROOT, 'watcher.log');
fs.mkdirSync(ROOT, { recursive: true });

function log(msg) {
  const line = `[${new Date().toISOString()}] ${msg}\n`;
  fs.appendFileSync(LOG, line);
  console.log(line.trim());
}

function body(req, cb) {
  const chunks = [];
  let size = 0;
  req.on('data', c => { size += c.length; if (size <= 2 * 1024 * 1024) chunks.push(c); });
  req.on('end', () => cb(Buffer.concat(chunks)));
}

http.createServer((req, res) => {
  const url = (req.url || '/').split('?')[0];
  if (req.method === 'GET' && url === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify({ ok: true, time: new Date().toISOString() }));
  }
  if (req.method === 'POST' && url === '/feedback') {
    body(req, raw => {
      let text = raw;
      try {
        if (raw.length >= 2 && raw[0] === 0xFF && raw[1] === 0xFE) text = raw.slice(2).toString('utf16le');
        else if (raw.length >= 2 && raw[0] === 0xFE && raw[1] === 0xFF) {
          const sw = Buffer.alloc(raw.length - 2);
          for (let i = 2; i + 1 < raw.length; i += 2) { sw[i-2] = raw[i+1]; sw[i-1] = raw[i]; }
          text = sw.toString('utf16le');
        } else {
          text = raw.toString('utf8');
          if (text.includes('\uFFFD')) { try { text = new TextDecoder('gbk').decode(raw); } catch {} }
        }
      } catch {}
      let data;
      try { data = JSON.parse(text); } catch { data = { text }; }
      const id = String(data.id || Date.now()).replace(/[^0-9A-Za-z_-]/g, '');
      const when = data.time || new Date().toLocaleString('zh-CN', { hour12: false });
      const content = [
        'MCL 启动器 BUG 反馈',
        `时间: ${when}`,
        `版本: ${data.version || 'unknown'}`,
        `联系方式: ${data.contact || ''}`,
        '来源: Win10 服务器 192.168.3.173:7891',
        '-'.repeat(48),
        String(data.text || ''),
        ''
      ].join('\r\n');
      const file = path.join(ROOT, `BUG反馈_${id}.txt`);
      fs.writeFileSync(file, content, 'utf8');
      fs.writeFileSync(path.join(ROOT, 'BUG反馈.txt'), content, 'utf8');
      fs.appendFileSync(path.join(ROOT, 'pending.jsonl'), JSON.stringify({ id, time: when, file, content }) + '\n');
      fs.writeFileSync(path.join(ROOT, 'PENDING'), id, 'utf8');
      log(`received feedback ${id} (${String(data.text || '').length} chars) -> ${file}`);
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ ok: true, id }));
    });
    return;
  }
  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end('not found');
}).listen(PORT, '0.0.0.0', () => log(`feedback receiver listening on ${PORT}`));
