const body = new URLSearchParams({feedback: '【限流自测】这是一条用于验证同 IP 60 秒限流的测试反馈。', contact: 'DSH'});
(async () => {
  for (let i=1;i<=2;i++) {
    const r = await fetch('http://192.168.3.173:7891/feedback', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'}, body});
    const text = await r.text();
    console.log('#'+i, r.status, text.slice(0,80).replace(/\n/g,' '));
  }
})().catch(e=>{console.error(e);process.exit(1)});
