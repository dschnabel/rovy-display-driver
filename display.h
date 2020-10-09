#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifdef __cplusplus
extern "C"{
#endif

int dd_init();
void dd_destroy();

/*
 * build compatible video:
 * ffmpeg -r 60 -s 800x480 -i f%d.jpg -vcodec libx264 -crf 25 -pix_fmt yuv420p -acodec aac output.h264
 */
int dd_play_video(unsigned char *buffer, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H_ */
