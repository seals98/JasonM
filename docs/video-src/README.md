30 s boot-flow video (1920x1080, 30 fps). `scene.html` draws the frame for time `t` (`draw(t)`);
`render.js` captures 900 frames with Playwright and pipes them to ffmpeg:
`node render.js <abs path to this dir>`  ->  `boot_flow.mp4` in this dir (needs playwright + ffmpeg).
The log panel in the video is illustrative, not real UART output.
