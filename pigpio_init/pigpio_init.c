#include "m_pd.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pigpio.h>

static t_class *pigpio_init_class;

typedef struct _pigpio_init
{
    t_object x_obj;
    bool initialised;
} t_pigpio_init;

static void pigpio_init_free(t_pigpio_init *x)
{
    gpioTerminate();
    x->initialised = false;
    post("[pigpio_init] pigpio terminated");
}

static void pigpio_init_bang(t_pigpio_init *x)
{
    if (x->initialised)
    {
        return;
    }

    if (geteuid() != 0)
    {
        pd_error(x, "[pigpio_init] root is required to use this external");
        exit(0);
    }

    // Use PWM for timing DMA transfers, as PCM is used for audio
    gpioCfgClock(5, 0, 0);

    if (gpioInitialise() < 0)
    {
        pd_error(x, "[pigpio_init] Unable to initialise GPIO; ensure pigpio_initd isn't running");
        exit(0);
    }

    post("[pigpio_init] pigpio initialised. Using PWM for timing DMA transfers.");

    x->initialised = true;
}

static void *pigpio_init_new()
{
    t_pigpio_init *x = (t_pigpio_init *)pd_new(pigpio_init_class);
    x->initialised = false;
    
    return x;
}

void pigpio_init_setup(void)
{
    pigpio_init_class = class_new(gensym("pigpio_init"),
                                  (t_newmethod)pigpio_init_new,
                                  (t_method)pigpio_init_free,
                                  sizeof(t_pigpio_init),
                                  CLASS_DEFAULT,
                                  0);

    class_addbang(pigpio_init_class, pigpio_init_bang);
}
