#ifndef __PRIVOXY_EXPORTS_H__
#define __PRIVOXY_EXPORTS_H__ 1

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*privoxy_cb)(int listen_fd, void *data);
extern int privoxy_main_entry(const char *conf_path, privoxy_cb cb, void *data);
extern void privoxy_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRIVOXY_EXPORTS_H__ */
