// ---- Secrets ----
#ifndef UTILS_h
#define UTILS_h
#include <time.h>

typedef enum
{
    OFF = false,
    ON = true
} light_state_t;

typedef enum
{
    DAY,
    NIGHT
} day_time_t;

typedef struct
{
    int id;
    light_state_t state = OFF;
    bool changed_state = false;
    unsigned int port = 38899;
    char ip_addr[15];
} light_t;


#endif // UTILS_h