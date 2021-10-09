#include "m_pd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>

static t_class *pigpio_mcp3008_class;

typedef struct _pigpio_mcp3008
{
    t_object x_obj;
    t_outlet *x_out;
    int x_cs_gpio;
    int x_miso_gpio;
    int x_mosi_gpio;
    int x_sclk_gpio;

    int baud;

    // SPI flags
    int flags;

    // Number of MCP3008 pins we're reading
    int input_pins;

    bool initialised;

} t_pigpio_mcp3008;

static void pigpio_mcp3008_free(t_pigpio_mcp3008 *x)
{
    // Close SPI connection if required
    if (x->initialised)
    {
        bbSPIClose(x->x_cs_gpio);
        post("[pigpio_mcp3008] closing for cs:%d", x->x_cs_gpio);
    }

    outlet_free(x->x_out);
    gpioTerminate();

    x->initialised = false;
}

static void pigpio_mcp3008_bang(t_pigpio_mcp3008 *x)
{
    // Initialise SPI on first bang
    if (!x->initialised)
    {
        bbSPIOpen(x->x_cs_gpio, x->x_miso_gpio, x->x_mosi_gpio, x->x_sclk_gpio, x->baud, x->flags);
        x->initialised = true;

        post("[pigpio_mcp3008] initialised for cs:%d", x->x_cs_gpio);
    }
    
    unsigned char buffer[3];
    char command[3];

    command[0] = 1;
    command[2] = 0;

    t_atom values[x->input_pins];
    float value;

    // Read from all MCP3008 input pins
    for (int i = 0; i < x->input_pins; i++)
    {
        command[1] = (i + 8) << 4;
        bbSPIXfer(x->x_cs_gpio, (char *)command, (char *)buffer, 3);

        // Extract value, normalise to 0-1
        value = (((buffer[1] & 3) << 8) | buffer[2]) / 1023.0f;
        SETFLOAT(&values[i], value);
    }

    // Output all values as a list
    outlet_list(x->x_out, 0, x->input_pins, values);
}

static void *pigpio_mcp3008_new(t_floatarg f1, t_floatarg f2, t_floatarg f3, t_floatarg f4)
{
    t_pigpio_mcp3008 *x = (t_pigpio_mcp3008 *)pd_new(pigpio_mcp3008_class);

    // Assign arguments
    x->x_cs_gpio = f1;
    x->x_miso_gpio = f2;
    x->x_mosi_gpio = f3;
    x->x_sclk_gpio = f4;

    // Set constants
    x->baud = 250000;
    x->flags = 0;
    x->input_pins = 8;

    x->x_out = outlet_new(&x->x_obj, gensym("list"));
    x->initialised = false;

    post("[pigpio_mcp3008] cs:%d, miso:%d, mosi:%d, sclk:%d", x->x_cs_gpio, x->x_miso_gpio, x->x_mosi_gpio, x->x_sclk_gpio);

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
