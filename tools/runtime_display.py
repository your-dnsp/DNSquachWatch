"""Narrow, fail-closed adapter for pinned TFT_eSPI 2.5.43, 2.8-inch CYD ST7789 / ILI9341 only."""
def patch(source):
    replacements = {
        'SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE)': ('SPISettings(dnsp_display_write_hz(), MSBFIRST, TFT_SPI_MODE)', 2),
        'spi.setFrequency(SPI_FREQUENCY);': ('spi.setFrequency(dnsp_display_write_hz());', 1),
        'tft_settings.tft_spi_freq = SPI_FREQUENCY/100000;': ('tft_settings.tft_spi_freq = dnsp_display_write_hz()/100000;', 1),
    }
    for old, (new, count) in replacements.items():
        if source.count(old) != count:
            raise RuntimeError('TFT_eSPI write-clock adapter needs review: ' + old)
        source = source.replace(old, new)
    return '#include <stdint.h>\nextern "C" uint32_t dnsp_display_write_hz();\n' + source
