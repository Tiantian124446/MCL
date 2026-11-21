const text = '启动器「关于」页的「提交 BUG 反馈」按钮，点开后只打开官网首页，反馈弹窗没有自动弹出，还要再点一次页面上的「BUG反馈」。请让这个按钮直接打开反馈窗口（可以用 /?feedback=1）。';
const body = new URLSearchParams({feedback: text, contact: 'DSH 自测'});
fetch('http://192.168.3.173:7891/feedback', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'}, body})
  .then(async r => { console.log('status', r.status, 'id', r.headers.get('x-feedback-id')); const t = await r.text(); console.log(t.split('\n').slice(-3).join('\n')); })
  .catch(e => { console.error('ERR', e.message); process.exit(1); });
