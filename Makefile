# You probably never need to adjust this Makefile.
# All changes can be done in the makefile.mk

#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

export PATH		:=	$(DEVKITPPC)/bin:$(PORTLIBS)/bin:$(PATH)
export PORTLIBS :=	$(DEVKITPRO)/portlibs/ppc
export WUPSDIR  :=  $(DEVKITPRO)/wups
export GCC_VER  :=  $(shell $(DEVKITPPC)/bin/powerpc-eabi-gcc -dumpversion)

PREFIX	:=	powerpc-eabi-

export AS		:=	$(PREFIX)as
export CC		:=	$(PREFIX)gcc
export CXX		:=	$(PREFIX)g++
export LD		:=	$(PREFIX)ld
export AR		:=	$(PREFIX)ar
export OBJCOPY	:=	$(PREFIX)objcopy

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:= 	build

ifeq ($(notdir $(CURDIR)),$(BUILD))
    include ../makefile.mk
else
    include makefile.mk
endif

include $(WUPSDIR)/plugin_makefile.mk


#MAP ?= $(TARGET:.mod=.map)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

# -Os: optimise size
# -Wall: generate lots of warnings
# -D__wiiu__: define the symbol __wiiu__ (used in some headers)
# -mcpu=750: enable processor specific compilation
# -meabi: enable eabi specific compilation
# -mhard-float: enable hardware floating point instructions
# -nostartfiles: Do not use the standard system startup files when linking
# -ffunction-sections: split up functions so linker can garbage collect
# -fdata-sections: split up data so linker can garbage collect
COMMON_CFLAGS	:= -Os -Wall -mcpu=750 -meabi -mhard-float  -D__WIIU__ -nostartfiles -ffunction-sections -fdata-sections -Wl,-q  $(COMMON_CFLAGS)

# -x c: compile as c code
# -std=c11: use the c11 standard
CFLAGS	  		:= $(COMMON_CFLAGS) -x c -std=gnu11 $(CFLAGS)

# -x c: compile as c++ code
# -std=gnu++11: use the c++11 standard
CXXFLAGS  		:= $(COMMON_CFLAGS) -x c++ -std=gnu++11 $(CXXFLAGS)

ifeq ($(DO_LOGGING), 1)
   CFLAGS += -D__LOGGING__
   CXXFLAGS += -D__LOGGING__
endif

#---------------------------------------------------------------------------------
# any extra ld flags
#--------------------------------------------------------------------------------
# --gc-sections: remove unneeded symbols
# -Map: generate a map file
LDFLAGS +=  -Wl,-Map,$(notdir $@).map,--gc-sections


#---------------------------------------------------------------------------------
Q := @
MAKEFLAGS += --no-print-directory
#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS    +=
#  
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	+=

NEEDS_WUT := 0
                    
ifeq ($(WUT_ENABLE_CPP), 1)
    WUT_ENABLE_NEWLIB    := 1
       
    LDFLAGS              += -Wl,-whole-archive,-lwutstdc++,-no-whole-archive
    NEEDS_WUT            := 1
endif

ifeq ($(WUT_ENABLE_NEWLIB), 1)     
    LDFLAGS              += -Wl,-whole-archive,-lwutnewlib,-no-whole-archive
    NEEDS_WUT            := 1
endif

ifeq ($(WUT_DEFAULT_MALLOC), 1)       
    LDFLAGS              += -Wl,-whole-archive,-lwutmalloc,-no-whole-archive
    NEEDS_WUT            := 1
endif

ifeq ($(NEEDS_WUT), 1)       
    ifeq ($(strip $(WUT_ROOT)),)
        $(error "Please set WUT_ROOT in your environment. export WUT_ROOT=<path to>wut)
    endif
    CFLAGS += -D__WUT__
    CXXFLAGS += -D__WUT__
endif

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export PROJECTDIR := $(CURDIR)
export OUTPUT	:=	$(CURDIR)/$(TARGETDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
TTFFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.ttf)))
PNGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.png)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export REAL_LD	:=	$(CC)
else
	export REAL_LD	:=	$(CXX)
endif

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o) \
					$(PNGFILES:.png=.png.o) $(addsuffix .o,$(BINFILES))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE_FULL	+=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                        $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                        $(EXTERNAL_INCLUDE)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS_FULL +=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
                        $(EXTERNAL_LIBPATHS)
                    

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean install

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).mod $(OUTPUT)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

###############################################################################
# Rule to make everything.
PHONY += all

all : $(OUTPUT)
###############################################################################
# Special build rules


# Rule to make the module file.
$(OUTPUT) : $(OFILES)
	@echo "linking ... " $@
	@$(REAL_LD) $(OFILES) $(LDFLAGS) $(LIBS) $(LIBPATHS_FULL) -o $@
  
###############################################################################
# Standard build rules
#---------------------------------------------------------------------------------
%.a:
#---------------------------------------------------------------------------------
	@echo $(notdir $@)
	@rm -f $@
	@$(AR) -rc $@ $^

#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(INCLUDE_FULL)  -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(INCLUDE_FULL)  -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.o: %.S
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(INCLUDE_FULL) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.png.o : %.png
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.jpg.o : %.jpg
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.ttf.o : %.ttf
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.bin.o : %.bin
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.wav.o : %.wav
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.mp3.o : %.mp3
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------
%.ogg.o : %.ogg
	@echo $(notdir $<)
	@bin2s -a 32 $< | $(AS) -o $(@)

###############################################################################
# Assembly listing rules

# Rule to make assembly listing.
PHONY += list
list  : $(LIST)

# Rule to make the listing file.
%.list : $(TARGET)
	$(LOG)
	-$Qmkdir -p $(dir $@)
	$Q$(OBJDUMP) -d $< > $@

###############################################################################
# Clean rule

# Rule to clean files.
PHONY += clean
clean : 
	$Qrm -rf $(wildcard $(BUILD) $(BIN))

###############################################################################
# Phony targets

.PHONY : $(PHONY)
  
-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
