#pragma once
#include "stefi_board.h"
#include <stdbool.h>

#include "libstefi/interrupts.h"

//Functions
void button_init(stefi_button_t id);
void button_interrupt_init(stefi_button_t id, callbackfn_t fn);
bool button_is_pressed(stefi_button_t);
