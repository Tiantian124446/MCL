const body = new URLSearchParams({feedback: '【浏览器自测】中文反馈编码测试：启动器下载与反馈按钮应当正常工作。', contact: 'DSH'});
fetch('http://192.168.3.173:7891/feedback', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'}, body})
  .then(async r => { console.log('status', r.status, 'disp', r.headers.get('content-disposition')); console.log((await r.text()).slice(0,200)); })
  .catch(e => { console.error('ERR', e.message); process.exit(1); });
