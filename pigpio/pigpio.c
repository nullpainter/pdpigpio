#include "m_pd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>

static t_class *pigpio_class;

typedef struct _pigpio
{
    t_object x_obj;
    t_inlet *in_intensity;
    t_outlet *x_out;
    int x_gpio;
    float x_val;
    unsigned int gpio_mode;
    bool initialised;

} t_pigpio;

static void pigpio_free(t_pigpio *x)
{
    gpioSetMode(x->x_gpio, PI_INPUT);
    gpioTerminate();

    inlet_free(x->in_intensity);
    outlet_free(x->x_out);
    x->initialised = false;
}

static void pigpio_initialise(t_pigpio *x)
{
    if (x->initialised)
    {
        return;
    }

    gpioSetMode(x->x_gpio, x->gpio_mode);

    x->initialised = true;
}

static void *pigpio_new(t_floatarg f1, t_floatarg f2)
{
    t_pigpio *x = (t_pigpio *)pd_new(pigpio_class);

    // Assign arguments
    x->x_gpio = f1;
    x->x_val = f2;

    post("[pigpio] gpio:%d", x->x_gpio);

    x->in_intensity = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("intensity"));
    x->x_out = outlet_new(&x->x_obj, &s_float);

    return (void *)x;
}


static void pigpio_bang(t_pigpio *x)
{
    if (!x->initialised)
    {
        x->gpio_mode = PI_INPUT;
        pigpio_initialise(x);
    }

    int val = gpioRead(x->x_gpio);
    outlet_float(x->x_out, val);
}

static void pigpio_list(t_pigpio *x, t_symbol *s, t_int argc, t_atom *argv) 
{
    if (argc != 1) 
    {
        pd_error(0, "[pigpio] single gpio argument required");
        return;
    }

    if (x->x_gpio != 0 && x->gpio_mode == PI_OUTPUT) 
    {
        // Reset previous state
        gpioSetMode(x->x_gpio, PI_INPUT);
    }

    x->x_gpio = atom_getfloat(argv);
    gpioSetMode(x->x_gpio, x->gpio_mode);

    post("[pigpio] set gpio to %d", x->x_gpio);
}

void pigpio_set_intensity(t_pigpio *x, t_floatarg f)
{
    // Clamp
    f = f < 0 ? 0 : f;
    f = f > 1 ? 1 : f;

    x->x_val = f;

    if (!x->initialised)
    {
        x->gpio_mode = PI_OUTPUT;
        pigpio_initialise(x);
    }

    gpioWrite(x->x_gpio, x->x_val);
}

void pigpio_setup(void)
{
    pigpio_class = class_new(gensym("pigpio"),
                             (t_newmethod)pigpio_new,
                             (t_method)pigpio_free,
                             sizeof(t_pigpio),
                             CLASS_DEFAULT,
                             A_DEFFLOAT,
                             0);

    class_addmethod(
        pigpio_class,
        (t_method)pigpio_set_intensity,
        gensym("intensity"),
        A_DEFFLOAT,
        0);

    class_addlist(pigpio_class, (t_method)pigpio_list);

    class_addbang(pigpio_class, pigpio_bang);
}
