const body = new URLSearchParams({feedback: '【CSV自测】这是一条用于验证反馈 CSV 导出的测试内容。', contact: 'DSH'});
fetch('http://192.168.3.173:7891/feedback', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'}, body})
  .then(r=>{console.log('submit', r.status); return r.text();}).then(t=>console.log(t.slice(0,40))).catch(e=>{console.error(e);process.exit(1)});
