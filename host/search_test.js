async function submit(text){
  const body = new URLSearchParams({feedback: text, contact:'DSH'});
  const r = await fetch('http://192.168.3.173:7891/feedback',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body});
  console.log('submit', r.status, text);
}
await submit('【搜索自测A】alpha_marker 内容');
await submit('【搜索自测B】beta_marker 内容');
