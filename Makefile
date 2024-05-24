# Turn on the optimizer
OPT = -Os

# Print Floats
LDFLAGS = -u _printf_float

# Project Name
TARGET = Main

# Sources
CPP_SOURCES = Main.cpp DelayVoice.cpp DelayEngine.cpp

# Library Locations
LIBDAISY_DIR = /home/luca/Desktop/DaisyExamples/libDaisy/
DAISYSP_DIR = /home/luca/Desktop/DaisyExamples/DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
