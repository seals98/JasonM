const {chromium}=require('playwright'); const {spawn}=require('child_process');
(async()=>{
  const dir=process.argv[2], FPS=30, DUR=30, N=FPS*DUR;
  const b=await chromium.launch(); const p=await b.newPage({viewport:{width:1920,height:1080}});
  await p.goto('file://'+dir+'/scene.html');
  const ff=spawn('ffmpeg',['-loglevel','error','-y','-f','image2pipe','-framerate',String(FPS),'-i','-',
    '-c:v','libx264','-pix_fmt','yuv420p','-crf','20','-preset','medium','-movflags','+faststart',dir+'/boot_flow.mp4'],{stdio:['pipe','inherit','inherit']});
  const done=new Promise(r=>ff.on('close',r));
  const t0=Date.now();
  for(let i=0;i<N;i++){
    await p.evaluate(t=>draw(t),i/FPS);
    const buf=await p.screenshot({type:'png'});
    if(!ff.stdin.write(buf)) await new Promise(r=>ff.stdin.once('drain',r));
    if(i%150===0) console.log('frame',i,((Date.now()-t0)/1000).toFixed(0)+'s');
  }
  ff.stdin.end(); await done; await b.close(); console.log('done',((Date.now()-t0)/1000).toFixed(0)+'s');
})();
