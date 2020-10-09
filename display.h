#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifdef __cplusplus
extern "C"{
#endif

int dd_init();
void dd_destroy();
int dd_display(unsigned char *buffer, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H_ */
