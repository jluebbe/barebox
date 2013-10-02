#ifndef __ASM_ARCH_LINUX_H
#define __ASM_ARCH_LINUX_H

struct device_d;

int sandbox_add_device(struct device_d *dev);

struct fb_bitfield;

int linux_register_device(const char *name, void *start, void *end);
int tap_alloc(char *dev);
uint64_t linux_get_time(void);
int linux_read(int fd, void *buf, size_t count);
int linux_read_nonblock(int fd, void *buf, size_t count);
ssize_t linux_write(int fd, const void *buf, size_t count);
off_t linux_lseek(int fildes, off_t offset);
int linux_tstc(int fd);

int linux_execve(const char * filename, char *const argv[], char *const envp[]);

int barebox_register_console(char *name_template, int stdinfd, int stdoutfd);

struct linux_console_data {
	int stdinfd;
	int stdoutfd;
};

extern int sdl_xres;
extern int sdl_yres;
int sdl_init(void);
void sdl_close(void);
int sdl_open(int xres, int yres, int bpp, void* buf);
void sdl_stop_timer(void);
void sdl_start_timer(void);
void sdl_get_bitfield_rgba(struct fb_bitfield *r, struct fb_bitfield *g,
			    struct fb_bitfield *b, struct fb_bitfield *a);
void sdl_setpixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif /* __ASM_ARCH_LINUX_H */
