#ifndef IXFILM_STATUS_H
#define IXFILM_STATUS_H
/* state: 1 developing, 2 done, 3 error, 4 boot hint, 5 selected film,
 * 6 queue complete + selected film, 7 queue active, 8 queue aborted */
void ixfilm_status_update(const char *stock, int percent,
                          int current, int total, int state);
void ixfilm_boot_hint(void);
void ixfilm_screen_activity(void);
int ixfilm_conversion_is_active(void);
#endif
