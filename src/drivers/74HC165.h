#pragma once

#include "per/gpio.h"

class Driver74HC165
{
  public:
    struct Config
    {
        dsy_gpio_pin clk;        /**< Clock pin to attach to pin 2 of device */
        dsy_gpio_pin load;       /**< Latch pin to attach to pin 1 of device */
        dsy_gpio_pin data;       /**< Data pin (pin 9) */
        dsy_gpio_pin clk_enable; /**< Clock enable pin (pin 15) */
    };

    Driver74HC165() {}
    ~Driver74HC165() {}

    void Init(const Config& cfg)
    {
        config_ = cfg;

        // Init GPIO
        clk_.mode = DSY_GPIO_MODE_OUTPUT_PP;
        clk_.pull = DSY_GPIO_NOPULL;
        clk_.pin  = cfg.clk;
        dsy_gpio_init(&clk_);

        load_.mode = DSY_GPIO_MODE_OUTPUT_PP;
        load_.pull = DSY_GPIO_NOPULL;
        load_.pin  = cfg.load;
        dsy_gpio_init(&load_);

        clk_enable_.mode = DSY_GPIO_MODE_OUTPUT_PP;
        clk_enable_.pull = DSY_GPIO_NOPULL;
        clk_enable_.pin  = cfg.clk_enable;
        dsy_gpio_init(&clk_enable_);

        data_.mode = DSY_GPIO_MODE_INPUT;
        data_.pull = DSY_GPIO_NOPULL;
        data_.pin  = cfg.data;
        dsy_gpio_init(&data_);

        state_ = 0;
    }

    void Update()
    {
        // Reset state to zero
        state_ = 0;

        dsy_gpio_write(&clk_, 0);

        // Parallel shift in (load low)
        dsy_gpio_write(&load_, 0);

        // Serial shift out (load high)
        dsy_gpio_write(&load_, 1);

        bool val = 0;

        for(size_t i = 0; i < 16; i++)
        {
            dsy_gpio_write(&clk_, 0);

            val               = dsy_gpio_read(&data_);
            uint8_t shifted   = val << i;
            uint8_t new_state = state_ | shifted;
            state_            = new_state;

            dsy_gpio_write(&clk_, 1);
        }
    }


    uint8_t State() const { return state_; }

    const Config& GetConfig() const { return config_; }

  private:
    Config  config_;
    uint8_t state_;

    dsy_gpio clk_;
    dsy_gpio load_;
    dsy_gpio data_;
    dsy_gpio clk_enable_;
};