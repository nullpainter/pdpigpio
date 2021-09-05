#include "m_pd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>

static t_class *pigpio_mcp3008_class;
static bool initialised = false;

static unsigned char buffer[3];
static char command[3];

#define BAUD 10000
#define FLAGS 0 

// Number of MCP3008 pins we're reading
#define INPUT_PINS 8

typedef struct _pigpio_mcp3008
{
    t_object x_obj;
    t_outlet *x_out;
    int x_cs_pin;
    int x_miso_pin;
    int x_mosi_pin;
    int x_sclk_pin;

} t_pigpio_mcp3008;

static void pigpio_mcp3008_free(t_pigpio_mcp3008 *x)
{
    // Close SPI connection
    bbSPIClose(x->x_cs_pin);
}

static void pigpio_mcp3008_initialise(t_pigpio_mcp3008 *x)
{
    if (initialised)
    {
        return;
    }

    // Open SPI connection
    bbSPIOpen(x->x_cs_pin, x->x_miso_pin, x->x_mosi_pin, x->x_sclk_pin, BAUD, FLAGS);
    initialised = true;
}

static void pigpio_mcp3008_bang(t_pigpio_mcp3008 *x)
{
    // Initialise SPI on first bang
    if (!initialised)
    {
        pigpio_mcp3008_initialise(x);
    }

    command[0] = 1;
    command[2] = 0;

    t_atom values[INPUT_PINS];
    float value;

    // Read from all MCP3008 input pins
    for (int i = 0; i < INPUT_PINS; i++) 
    {
        command[1] = (i + 8) << 4;
        bbSPIXfer(x->x_cs_pin, command, (char *)buffer, 3);

        // Extract value, normalise to 0-1
        value = (((buffer[1] & 3) << 8) | buffer[2]) / 1023.0f;
        SETFLOAT(&values[i], value);
    }

    // Output all values as a list
    outlet_list(x->x_out, 0, INPUT_PINS, values);
}

static void *pigpio_mcp3008_new(t_floatarg f1, t_floatarg f2, t_floatarg f3, t_floatarg f4)
{
    if (initialised) 
    {
       initialised = false;
    }

    t_pigpio_mcp3008 *x = (t_pigpio_mcp3008 *)pd_new(pigpio_mcp3008_class);

    // Assign arguments
    x->x_cs_pin = f1;
    x->x_miso_pin = f2;
    x->x_mosi_pin = f3;
    x->x_sclk_pin = f4;

    post("[pigpio_mcp3008] cs:%d, miso:%d, mosi:%d, sclk:%d", x->x_cs_pin, x->x_miso_pin, x->x_mosi_pin, x->x_sclk_pin);

    if (geteuid() != 0)
    {
        pd_error(x, "[pigpio_mcp3008] root is required to use this external");
        exit(0);
    }

    if (gpioInitialise() < 0)
    {
        pd_error(x, "[pigpio_mcp3008] Unable to initialise GPIO; ensure pigpiod isn't running");
        exit(0);
    }

    x->x_out = outlet_new(&x->x_obj, gensym("list"));

    return x;
}

void pigpio_mcp3008_setup(void)
{
    pigpio_mcp3008_class = class_new(gensym("pigpio_mcp3008"),
                                     (t_newmethod)pigpio_mcp3008_new,
                                     (t_method)pigpio_mcp3008_free,
                                     sizeof(t_pigpio_mcp3008),
                                     CLASS_DEFAULT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     0);

    class_addbang(pigpio_mcp3008_class, pigpio_mcp3008_bang);
}