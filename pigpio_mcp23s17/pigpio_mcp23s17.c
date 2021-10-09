#include "m_pd.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>

static t_class *pigpio_mcp23s17_class;

#define PIGPIO_MCP23S17_WRITE_COMMAND 0x40;
#define PIGPIO_MCP23S17_READ_COMMAND  0x41;

#define PIGPIO_MCP23S17_GPPUA   0x0C   // Pull up resistors, bank A
#define PIGPIO_MCP23S17_GPIOA   0x12   // GPIO bank A
#define PIGPIO_MCP23S17_GPIOB   0x13   // GPIO bank 

typedef struct _pigpio_mcp23s17
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

    bool initialised;

} t_pigpio_mcp23s17;


static void pigpio_mcp23s17_free(t_pigpio_mcp23s17 *x)
{
    // Close SPI connection if required
    if (x->initialised)
    {
        bbSPIClose(x->x_cs_gpio);
        post("[pigpio_mcp23s17] closing for cs:%d", x->x_cs_gpio);
    }

    outlet_free(x->x_out);
    gpioTerminate();

    x->initialised = false;
}

/*
 * Enables pull-up resistors on all pins.
 */
void pigpio_mcp23s17_enable_pullup(t_pigpio_mcp23s17 *x)
{
    unsigned char buffer[4];
    char command[4];

    command[0] = PIGPIO_MCP23S17_WRITE_COMMAND;
    command[1] = PIGPIO_MCP23S17_GPPUA;
    command[2] = 0xFF; // Enable on all GPIO pins, GPPUA
    command[3] = 0xFF; // GPPUB 

    bbSPIXfer(x->x_cs_gpio, command, (char *)buffer, sizeof(command)); 
}


static void pigpio_mcp23s17_bang(t_pigpio_mcp23s17 *x)
{
    // Initialise SPI and IC on first bang
    if (!x->initialised)
    {
        bbSPIOpen(x->x_cs_gpio, x->x_miso_gpio, x->x_mosi_gpio, x->x_sclk_gpio, x->baud, x->flags);
        pigpio_mcp23s17_enable_pullup(x);
        x->initialised = true;

        post("[pigpio_mcp23s17] initialised for cs:%d", x->x_cs_gpio);
    }

    unsigned char buffer[4];
    char command[4];

    t_atom values[16];

    // Read from both banks
    command[0] = PIGPIO_MCP23S17_READ_COMMAND;
    command[1] = PIGPIO_MCP23S17_GPIOA;
    command[2] = PIGPIO_MCP23S17_GPIOB;
    command[3] = 0;

    bbSPIXfer(x->x_cs_gpio, command, (char *)buffer, sizeof(command)); 

    // Pin values are present in buffer[2] and buffer[3].  Store both banks in a single combined value.
    int combinedValue = (buffer[2] << 8) | buffer[3];

    // Convert to bits
    for (int i = 0; i < 16; i++)
    {
        // Invert bit value so 1 is on
        char value = (combinedValue & (1 << i)) != 0 ? 0 : 1;
        SETFLOAT(&values[i], value);
    }

    // Output all values as a list
    outlet_list(x->x_out, 0, 16, values);
}

static void *pigpio_mcp23s17_new(t_floatarg f1, t_floatarg f2, t_floatarg f3, t_floatarg f4)
{
    t_pigpio_mcp23s17 *x = (t_pigpio_mcp23s17 *)pd_new(pigpio_mcp23s17_class);

    // Assign arguments
    x->x_cs_gpio = f1;
    x->x_miso_gpio = f2;
    x->x_mosi_gpio = f3;
    x->x_sclk_gpio = f4;

    // Set constants
    x->baud = 250000;
    x->flags = 0;

    x->x_out = outlet_new(&x->x_obj, gensym("list"));
    x->initialised = false;

    post("[pigpio_mcp23s17] cs:%d, miso:%d, mosi:%d, sclk:%d", x->x_cs_gpio, x->x_miso_gpio, x->x_mosi_gpio, x->x_sclk_gpio);

    return x;
}

void pigpio_mcp23s17_setup(void)
{
    pigpio_mcp23s17_class = class_new(gensym("pigpio_mcp23s17"),
                                     (t_newmethod)pigpio_mcp23s17_new,
                                     (t_method)pigpio_mcp23s17_free,
                                     sizeof(t_pigpio_mcp23s17),
                                     CLASS_DEFAULT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     A_DEFFLOAT,
                                     0);

    class_addbang(pigpio_mcp23s17_class, pigpio_mcp23s17_bang);
}
