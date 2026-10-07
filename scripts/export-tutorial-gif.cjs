// Development-only export: render the reviewed HTML at its own aspect ratio.
// Usage: node scripts/export-tutorial-gif.cjs [path-to-chromium] [--fps=50] [--speed=2.5] [--width=1440]
// Optional: --scenes=window,text --frames-dir=build/tutorial-frames
// Requires Node.js and Playwright; neither is required by the Windows utility.
const {chromium}=require('playwright');
const fs=require('node:fs'),path=require('node:path');
const {pathToFileURL}=require('node:url');
const root=path.resolve(__dirname,'..');
const args=process.argv.slice(2);
const option=(name,fallback)=>args.find(a=>a.startsWith('--'+name+'='))?.split('=').slice(1).join('=')??fallback;
const fps=Number(option('fps','50')),speed=Number(option('speed','2.5'));
const outputWidth=Number(option('width','1440')),logicalWidth=760;
const browserPath=args.find(a=>!a.startsWith('--'));
const allScenes=['window','text','select','capture','drop','resize'];
const scenes=option('scenes',allScenes.join(',')).split(',');
if(![10,20,25,50].includes(fps)||!Number.isFinite(speed)||speed<=0||speed>3||!Number.isInteger(outputWidth)||outputWidth<300||outputWidth>3840||scenes.some(s=>!allScenes.includes(s)))throw Error('Unsupported export options');
const out=path.resolve(root,option('frames-dir','build/tutorial-frames'));
(async()=>{
  const browser=await chromium.launch({headless:true,...(browserPath?{executablePath:browserPath}:{})});
  try{
    const page=await browser.newPage({viewport:{width:logicalWidth,height:1400},deviceScaleFactor:outputWidth/logicalWidth,colorScheme:'light'});
    const errors=[];page.on('pageerror',e=>errors.push(String(e)));
    await page.route(/^https?:/,r=>r.abort());
    await page.goto(pathToFileURL(path.join(root,'docs','tutorial.html')).href+'?theme=light&embed=1&autoplay=0');
    await page.evaluate(()=>document.fonts.ready);
    await page.evaluate(()=>{
      const e=document.querySelector('three-finger-demo');e.pause();
      const s=document.createElement('style');s.textContent='*{transition:none!important}.head-actions > div,.tab-progress{display:none!important}';e.shadowRoot.append(s);
      e.shadowRoot.querySelector('.card').dataset.playing='true';
    });
    fs.mkdirSync(out,{recursive:true});
    const perSceneMs=Math.round(8400/speed/20)*20;
    const endTick=perSceneMs*fps/1000;
    const set=new Set([0,.34,.72,.85,1.05,1.25,1.36,1.85,2.05,2.12,2.25,2.75,4.6,5.4,5.45,5.8,6.05,6.28,6.4,7.05,8].map(t=>Math.round(t/8.4*endTick)));
    for(let tick=0;tick<endTick;tick++){
      const t=tick/endTick*8.4;
      if([[0,1.36],[1.85,6.28],[6.4,7.05],[8,8.4]].some(([a,b])=>t>=a&&t<b))set.add(tick);
    }
    const times=[...set].sort((a,b)=>a-b).filter(t=>t<endTick);
    const frames=[];
    for(const scene of scenes){
      await page.evaluate(s=>document.querySelector('three-finger-demo').selectScene(s),scene);
      for(let i=0;i<times.length;i++){
        const t=times[i]/endTick*8.4;await page.evaluate(t=>document.querySelector('three-finger-demo').seek(t),t);
        const name=String(frames.length).padStart(4,'0')+'.png';
        await page.locator('three-finger-demo').screenshot({path:path.join(out,name)});
        frames.push({file:name,scene,time:t,durationMs:Math.round(((times[i+1]??endTick)-times[i])*1000/fps)});
      }
      console.log('Rendered '+scene+' ('+times.length+' frames)');
    }
    const text=await page.evaluate(()=>{const e=document.querySelector('three-finger-demo');e.selectScene('text');e.seek(6.7);const r=e.shadowRoot;return{naturalWidth:r.querySelector('#selection-line').getComputedTextLength(),highlightWidth:Number(r.querySelector('#text-highlight').getAttribute('width')),forcedTextWidth:r.querySelectorAll('[textLength],[lengthAdjust]').length};});
    if(errors.length||text.forcedTextWidth||Math.abs(text.naturalWidth-text.highlightWidth)>.01)throw Error('Tutorial rendering did not validate');
    const bounds=await page.locator('three-finger-demo').boundingBox();
    const firstPng=fs.readFileSync(path.join(out,frames[0].file));
    const size=[firstPng.readUInt32BE(16),firstPng.readUInt32BE(20)];
    if(size[0]!==outputWidth)throw Error('Incorrect raster width');
    const durationMs=frames.reduce((s,f)=>s+f.durationMs,0);
    if(durationMs!==perSceneMs*scenes.length||frames.some(f=>f.durationMs<20))throw Error('Unsafe GIF frame timing');
    fs.writeFileSync(path.join(out,'frames.json'),JSON.stringify({frames,text,errors,fps,speed,size,cssSize:[bounds.width,bounds.height],durationMs},null,2));
    console.log(JSON.stringify({frames:frames.length,durationMs,fps,speed,size,text,errors}));
  }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
