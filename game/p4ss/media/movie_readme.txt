the ffmpeg command I used to make the intro movie was:
ffmpeg -i INFILE.EXT -b:v 10000k -c:v libvpx -c:a libvorbis out.webm
