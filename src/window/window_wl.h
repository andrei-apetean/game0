#ifdef GAME0_LINUX
#include <stdint.h>

struct window_config;

int32_t window_get_data_size_wl();
int32_t window_startup_wl(void* self, struct window_config* cfg);
int32_t window_poll_events_wl(void* self);
void    window_teardown_wl(void* self);
void*   window_get_native_handle_wl(void* self);

#endif // GAME0_LINUX
