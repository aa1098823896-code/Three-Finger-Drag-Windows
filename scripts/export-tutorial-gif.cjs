// Development-only export: render the reviewed HTML at its own aspect ratio.
// Usage: node scripts/export-tutorial-gif.cjs [path-to-chromium]
// Requires Node.js and Playwright; neither is required by the Windows utility.
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path');
const {pathToFileURL}=require('node:url');
const root=path.resolve(__dirname,'..');
(async()=>{
  const browser=await chromium.launch({headless:true,...(process.argv[2]?{executablePath:process.argv[2]}:{})});
  try{
    const page=await browser.newPage({viewport:{width:760,height:1400},colorScheme:'light'});
    const errors=[];page.on('pageerror',e=>errors.push(String(e)));
    await page.route(/^https?:/,r=>r.abort());
    await page.goto(pathToFileURL(path.join(root,'docs','tutorial.html')).href+'?theme=light&embed=1&autoplay=0');
    await page.evaluate(()=>document.fonts.ready);
    await page.evaluate(()=>{
      const e=document.querySelector('three-finger-demo');e.pause();
      const s=document.createElement('style');s.textContent='*{transition:none!important}';e.shadowRoot.append(s);
      e.shadowRoot.querySelector('.head-actions > div').style.visibility='hidden';
      e.shadowRoot.querySelector('.card').dataset.playing='true';
    });
    const out=path.join(root,'build','tutorial-frames');fs.mkdirSync(out,{recursive:true});
    const scenes=['window','text','select','capture','drop','resize'];
    const set=new Set([0,.34,.72,.85,1.05,1.25,1.36,2.05,2.12,2.25,2.75,4.6,5.4,5.45,5.8,6.05,6.28,6.4,7.05,8]);
    for(const [a,b] of [[0,1.36],[2.05,5.45],[5.8,6.28],[6.4,7.05],[8,8.4]])
      for(let t=a;t<b;t+=1/20)set.add(Math.round(t*100000)/100000);
    const times=[...set].sort((a,b)=>a-b).filter(t=>t<8.4);
    const frames=[];
    for(const scene of scenes){
      await page.evaluate(s=>document.querySelector('three-finger-demo').selectScene(s),scene);
      for(let i=0;i<times.length;i++){
        const t=times[i];await page.evaluate(t=>document.querySelector('three-finger-demo').seek(t),t);
        const name=String(frames.length).padStart(4,'0')+'.png';
        await page.locator('three-finger-demo').screenshot({path:path.join(out,name)});
        frames.push({file:name,scene,time:t,durationMs:Math.round(((times[i+1]??8.4)-t)*1000)});
      }
      console.log('Rendered '+scene+' ('+times.length+' frames)');
    }
    const text=await page.evaluate(()=>{const e=document.querySelector('three-finger-demo');e.selectScene('text');e.seek(6.7);const r=e.shadowRoot;return{naturalWidth:r.querySelector('#selection-line').getComputedTextLength(),highlightWidth:Number(r.querySelector('#text-highlight').getAttribute('width')),forcedTextWidth:r.querySelectorAll('[textLength],[lengthAdjust]').length};});
    if(errors.length||text.forcedTextWidth||Math.abs(text.naturalWidth-text.highlightWidth)>.01)throw Error('Tutorial rendering did not validate');
    fs.writeFileSync(path.join(out,'frames.json'),JSON.stringify({frames,text,errors},null,2));
    console.log(JSON.stringify({frames:frames.length,durationMs:frames.reduce((s,f)=>s+f.durationMs,0),text,errors}));
  }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
