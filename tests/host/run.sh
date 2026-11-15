#!/bin/sh
# Host unit tests for the portable components and the oscilloscope's
# protocol/state machine. Needs a native C compiler only:
#   sh tests/host/run.sh
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/stefi-host-tests
mkdir -p "$OUT"
CC=${CC:-cc}

# cmd_line, dsp moving average, ADC sample conversion (vs. the V1 firmware)
$CC -Wall -O1 -I"$ROOT/components/cmd_line" -I"$ROOT/components/dsp" \
    "$ROOT/tests/host/test_components.c" \
    "$ROOT/components/cmd_line/cmd_line.c" "$ROOT/components/dsp/moving_average.c" \
    -o "$OUT/test_components"
"$OUT/test_components"

# the PC software's command sequence replayed against scope_fsm/scope_protocol
# (hardware actions stubbed; stubs/ shadows the libstefi headers)
$CC -Wall -O1 -I"$ROOT/tests/host/stubs" -I"$ROOT/examples/10-UsbOscilloscope" \
    -I"$ROOT/components/event_fsm" -I"$ROOT/components/cmd_line" -I"$ROOT/include" \
    "$ROOT/tests/host/test_scope_protocol.c" \
    "$ROOT/examples/10-UsbOscilloscope/scope_fsm.c" \
    "$ROOT/examples/10-UsbOscilloscope/scope_protocol.c" \
    "$ROOT/components/event_fsm/event_fsm.c" "$ROOT/components/cmd_line/cmd_line.c" \
    "$ROOT/src/ringbuffer.c" -o "$OUT/test_scope_protocol"
"$OUT/test_scope_protocol"

# FMAC software model (G431 FMAC example reference) vs. a float IIR
$CC -Wall -O1 -DSTEFI_MCU_STM32L4G4 -I"$ROOT/include" -I"$ROOT/src" \
    "$ROOT/tests/host/test_fmac_model.c" "$ROOT/src/fmac.c" -lm -o "$OUT/test_fmac_model"
"$OUT/test_fmac_model"
