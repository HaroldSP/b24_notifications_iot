# To-Do List

## Issues to Fix

- [ ] Suppress or resolve coredump warning: `E (279) esp_core_dump_flash: No core dump partition found!`
  - Current status: Coredump partition removed (12KB too small, 64KB recommended but can't fit before factory partition)
  - Log suppression in `main.cpp` is not fully working - error still appears in serial output
  - Need to find better way to suppress this error or accept it as harmless
