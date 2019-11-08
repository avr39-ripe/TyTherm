ARDUINO_LIBRARIES := OneWire ArduinoJson5

#### overridable rBoot options ####

## use rboot build mode
RBOOT_ENABLED		?= 1

## enable big flash support (for multiple roms, each in separate 1mb block of flash)
RBOOT_BIG_FLASH		?= 1

## two rom mode (where two roms sit in the same 1mb block of flash)
#RBOOT_TWO_ROMS		?= 1

## size of the flash chip
SPI_SIZE			?= 4M

## output file for first rom (.bin will be appended)
#RBOOT_ROM_0 		?= rom0

## input linker file for first rom
#RBOOT_LD_0			?= rom0.ld

## these next options only needed when using two rom mode
#RBOOT_ROM_1		?= rom1
#RBOOT_LD_1			?= rom1.ld


## option to completely disable spiffs
#DISABLE_SPIFFS		?= 1

## flash offsets for spiffs, set if using two rom mode or not on a 4mb flash
## (spiffs location defaults to the mb after the rom slot on 4mb flash)
#RBOOT_SPIFFS_0		?= 0x100000
#RBOOT_SPIFFS_1		?= 0x300000

## use rboot build mode
RBOOT_ENABLED ?= 1
## enable big flash support (for multiple roms, each in separate 1mb block of flash)
RBOOT_BIG_FLASH ?= 1
## two rom mode (where two roms sit in the same 1mb block of flash)
RBOOT_TWO_ROMS  ?= 0
## size of the flash chip
SPI_SIZE        ?= 4M
## output file for first rom (.bin will be appended)
#RBOOT_ROM_0     ?= rom0
## input linker file for first rom
#RBOOT_LD_0      ?= rom0.ld
## these next options only needed when using two rom mode
#RBOOT_ROM_1     ?= rom1
#RBOOT_LD_1      ?= rom1.ld
## size of the spiffs to create
#SPIFF_SIZE      ?= 65536
## option to completely disable spiffs
#DISABLE_SPIFFS  = 1
## flash offsets for spiffs, set if using two rom mode or not on a 4mb flash
## (spiffs location defaults to the mb after the rom slot on 4mb flash)
RBOOT_SPIFFS_0  ?= 0x100000
RBOOT_SPIFFS_1  ?= 0x300000

RBOOT_RTC_ENABLED = 1
SPIFF_SIZE      ?= 262144

COMPONENT_SRCDIRS = app lib/application lib/tempsensor lib/binio
COMPONENT_INCDIRS = include lib/application lib/tempsensor lib/binio lib/wsbinconst

ENABLE_CMD_EXECUTOR = 0

WEBPACK_OUT = files/index.*.js*

.PHONY: webpacker
webpacker: 
	$(vecho) "Bundling js with Webpack"
	$(Q) rm -rf $(WEBPACK_OUT)
	$(Q) webpack