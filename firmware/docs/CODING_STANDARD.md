# Coding Standard

## Naming Conventions

- **Files**: snake_case (z.B. `gpio_hal.c`, `app_config.h`)
- **Types**: snake_case mit `_e` fuer Enums, `_t` fuer Structs (z.B. `error_code_e`, `gpio_hal_cfg_t`)
- **Functions**: snake_case (z.B. `gpio_hal_init`)
- **Classes**: PascalCase (z.B. `App`)
- **Private Member**: `_` prefix (z.B. `_led_on`, `_led_cfg`)
- **Constants/Macros**: UPPER_SNAKE_CASE (z.B. `APP_BLINK_GPIO`, `APP_ERR_SUCCESS`)

## C/C++ Interoperability

All C headers must include `extern "C"` guards:

```c
#ifdef __cplusplus
extern "C" {
#endif

// declarations

#ifdef __cplusplus
}
#endif
```

## Error Handling

Use `error_code_e` from `error_codes.h` for all functions that can fail. Return `APP_ERR_SUCCESS` (0) on success, negative values on error. The `APP_ERR_` prefix avoids collision with lwIP error codes.

## Hardware Abstraction

All hardware access must go through HAL layers in `include/hw/` and `src/hw/`. Never access ESP-IDF drivers directly from application code.

## Formatting

Run `tools/format.sh` before committing. The project uses clang-format with the configuration in `.clang-format`.


## Randbedingungen

* Target: **ESP32-S3 (Seeed XIAO ESP32S3)**
* Build: **PlatformIO**
* Sprachsplit: **C für Treiber/HAL**, **C++ für Applikation/Architektur**
* Coding-Standard-Basis: **BARR-C (Style/Lesbarkeit) + MISRA-orientierte Regeln (Safety/Undefined Behavior vermeiden)**



## Praktische Regeln, die du im Projekt durchziehst

1. **Keine Magic Numbers**: Pins/Zeiten in `app_config.h` (Datenstand), Logik in `App` (Programmstand).
2. **Kein `strcpy/strcat`**: Längen immer prüfen (Buffer-Safety).
3. **Keine impliziten Casts**: Typen explizit, `stdint.h`-Typen bevorzugen.
4. **ISR kurz halten**: Event → Queue/Notification, Arbeit in Task.
5. **C++ nur als Embedded-Subset**: RAII ja, Exceptions eher nein; Fehler über Return-Code/`std::optional` (wenn genutzt).


## Deutsche Kommentare hinzugefuegt nach BARR-C Standard

- Datei-Header: @file, @brief mit Modulbeschreibung
- Sektionen: INCLUDES, TYPES, OEFFENTLICHE FUNKTIONEN, PRIVATE METHODEN
- Doxygen: @param, @return, @note fuer API-Dokumentation
- Inline: Erklaerungen bei nicht-trivialem Code (z.B. Nullzeiger-Pruefung, invertierte Logik)

