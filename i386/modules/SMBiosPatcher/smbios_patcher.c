/*
 * Copyright 2008 mackerintel
 */

#include "libsaio.h"
#include "boot.h"
#include "bootstruct.h"
#include "acpi.h"
#include "efi_tables.h"
#include "fake_efi.h"
#include "platform.h"
#include "smbios_patcher.h"
#include "pci.h"
#include "cpu.h"
#include "modules.h"

#ifndef DEBUG_SMBIOS
#define DEBUG_SMBIOS 0
#endif

#if DEBUG_SMBIOS==2
#define DBG(x...)	printf(x)
#elif DEBUG_SMBIOS==1
#define DBG(x...) msglog(x)
#else
#define DBG(x...)	
#endif

char* gSMBIOSBoardModel;

typedef struct {
    const char* key;
    const char* value;
} SMStrEntryPair;

// defaults for a MacBook
static const SMStrEntryPair const sm_macbook_defaults[]={
	{"SMbiosvendor",	"Apple Inc."			},
	{"SMbiosversion",	"MB41.88Z.00C1.B00.0802091535"	},
	{"SMbiosdate",		"02/09/2008"			},
	{"SMmanufacter",	"Apple Inc."			},
	{"SMproductname",	"MacBook4,1"			},
	{"SMsystemversion",	"1.0"				},
	{"SMserial",		"RM83064H0P1"			},
	{"SMfamily",		"MacBook"			},
	{"SMboardmanufacter",	"Apple Inc."			},
	{"SMboardproduct",	"Mac-F22788A9"			},
	{ "",""	}
};

// defaults for a MacBook Pro
static const SMStrEntryPair const sm_macbookpro_defaults[]={
	{"SMbiosvendor",	"Apple Inc."			},
	{"SMbiosversion",	"MBP41.88Z.00C1.B03.0802271651"	},
	{"SMbiosdate",		"02/27/2008"			},
	{"SMmanufacter",	"Apple Inc."			},
	{"SMproductname",	"MacBookPro4,1"			},
	{"SMsystemversion",	"1.0"				},
	{"SMserial",		"W88198N6YJX"			},
	{"SMfamily",		"MacBookPro"			},
	{"SMboardmanufacter",	"Apple Inc."			},
	{"SMboardproduct",	"Mac-F42C89C8"			},
	{ "",""	}
};

// defaults for a Mac mini 
static const SMStrEntryPair const sm_macmini_defaults[]={
	{"SMbiosvendor",	"Apple Inc."			},
	{"SMbiosversion",	"MM21.88Z.009A.B00.0706281359"	},
	{"SMbiosdate",		"06/28/2007"			},
	{"SMmanufacter",	"Apple Inc."			},
	{"SMproductname",	"Macmini2,1"			},
	{"SMsystemversion",	"1.0"				},
	{"SMserial",		"YM8054BYYL2"			},
	{"SMfamily",		"Napa Mac"			},
	{"SMboardmanufacter",	"Apple Inc."			},
	{"SMboardproduct",	"Mac-F4208EAA"			},
	{ "",""	}
};

// defaults for an iMac
static const SMStrEntryPair const sm_imac_defaults[]={
	{"SMbiosvendor",	"Apple Inc."			},
	{"SMbiosversion",	"IM71.88Z.007A.B03.0803051705"	},
	{"SMbiosdate",		"03/05/2008"			},
	{"SMmanufacter",	"Apple Inc."			},
	{"SMproductname",	"iMac7,1"			},	
	{"SMsystemversion",	"1.0"				},
	{"SMserial",		"W87410PWX87"			},
	{"SMfamily",		"Mac"				},
	{"SMboardmanufacter",	"Apple Inc."			},
	{"SMboardproduct",	"Mac-F4238CC8"			},
	{ "",""	}
};

// defaults for a Mac Pro
static const SMStrEntryPair const sm_macpro_defaults[]={
	{"SMbiosvendor",		"Apple Computer, Inc."			},
	{"SMbiosversion",		"MP31.88Z.006C.B02.0801021250"	},
	{"SMbiosdate",			"01/02/2008"					},
	{"SMmanufacter",		"Apple Computer, Inc."			},
	{"SMproductname",		"MacPro3,1"						},
	{"SMsystemversion",		"1.0"							},
	{"SMserial",			"G88014V4XYK"					},
	{"SMfamily",			"MacPro"						},
	{"SMboardmanufacter",	"Apple Computer, Inc."			},
	{"SMboardproduct",		"Mac-F42C88C8"					},
	{ "",""	}
};

// defaults for an iMac11,1 core i3/i5/i7
static const SMStrEntryPair const sm_imac_core_defaults[]={
	{"SMbiosvendor",		"Apple Inc."					},
	{"SMbiosversion",		"IM111.88Z.0034.B00.0910301727"	},
	{"SMbiosdate",			"10/30/2009"					},
	{"SMmanufacter",		"Apple Inc."					},
	{"SMproductname",		"iMac11,1"						},	
	{"SMsystemversion",		"1.0"							},
	{"SMserial",			"W89470DZ5RU"					},
	{"SMfamily",			"iMac"							},
	{"SMboardmanufacter",	"Apple Inc."                    },
	{"SMboardproduct",		"Mac-F2268DAE"					},
	{ "",""	}
};

// defaults for an iMac12,1 : todo: populate correctly 
static const SMStrEntryPair const sm_imac_sandy_defaults[]={
	{"SMbiosvendor",		"Apple Inc."					},
	{"SMbiosversion",		"IM121.88Z.0047.B00.1102091756"	},
	{"SMbiosdate",			"10/30/2011"					},
	{"SMmanufacter",		"Apple Inc."					},
	{"SMproductname",		"iMac12,1"						},	
	{"SMsystemversion",		"1.0"							},
	{"SMserial",			"W89470DZ5RU"					},
	{"SMfamily",			"iMac"							},
	{"SMboardmanufacter",	"Apple Inc."                    },
	{"SMboardproduct",		"Mac-F2268DAE"					},
	{ "",""	}
};

// defaults for a Mac Pro 4,1 core i7/Xeon
static const SMStrEntryPair const sm_macpro_core_defaults[]={
	{"SMbiosvendor",		"Apple Computer, Inc."			},
	{"SMbiosversion",		"MP41.88Z.0081.B03.0902231259"	},
	{"SMbiosdate",			"02/23/2009"					},
	{"SMmanufacter",		"Apple Inc."                    },
	{"SMproductname",		"MacPro4,1"						},
	{"SMsystemversion",		"1.0"							},
	{"SMserial",			"CK91601V8Q0"					},
	{"SMfamily",			"MacPro"						},
	{"SMboardmanufacter",	"Apple Computer, Inc."			},
	{"SMboardproduct",		"Mac-F221BEC8"					},
	{ "",""	}
};

// default for a Xserve
static const SMStrEntryPair const sm_xserve_defaults[]={
    {"SMbiosvendor",		"Apple Inc."					},
    {"SMbiosversion",		"XS21.88Z.006C.B06.0804011317"	},
    {"SMbiosdate",			"04/01/2008"					},
    {"SMmanufacter",		"Apple Inc."					},
    {"SMproductname",		"Xserve2,1"						},
    {"SMsystemversion",		"1.0"							},
    {"SMserial",			"CK816033X8S"					},
    {"SMfamily",			"Xserve"						},
    {"SMboardmanufacter",	"Apple Inc."					},
    {"SMboardproduct",		"Mac-F42289C8"					},
 	{ "",""	}
};

const char* sm_get_defstr(const char * key, int table_num)
{
	int	i;
	const SMStrEntryPair*	sm_defaults;

	if (Platform->CPU.isServer == true)
    {
     		sm_defaults=sm_xserve_defaults;
    } else if (Platform->CPU.isMobile == true) {
		if (Platform->CPU.NoCores > 1) {
			sm_defaults=sm_macbookpro_defaults;
		} else {
			sm_defaults=sm_macbook_defaults;
		}
	} else {
		switch (Platform->CPU.NoCores) 
		{
			case 1: 
				sm_defaults=sm_macmini_defaults; 
				break;
			case 2:
				sm_defaults=sm_imac_defaults;
				break;
			default:
			{
				switch (Platform->CPU.Family) 
				{
					case 0x06:
					{
						switch (Platform->CPU.Model)
						{
							case CPUID_MODEL_FIELDS: // Intel Core i5, i7 LGA1156 (45nm)
							case CPUID_MODEL_DALES: // Intel Core i5, i7 LGA1156 (45nm) ???
							case CPUID_MODEL_DALES_32NM: // Intel Core i3, i5, i7 LGA1156 (32nm) (Clarkdale, Arrandale)
							case 0x19: // Intel Core i5 650 @3.20 Ghz 
								sm_defaults=sm_imac_core_defaults; 
								break;
                                
                            case CPUID_MODEL_SANDYBRIDGE:
                            case CPUID_MODEL_JAKETOWN:
                                sm_defaults=sm_imac_sandy_defaults;
                                break;
                                
							case CPUID_MODEL_NEHALEM: 
							case CPUID_MODEL_NEHALEM_EX:
							case CPUID_MODEL_WESTMERE: 
							case CPUID_MODEL_WESTMERE_EX:
								sm_defaults=sm_macpro_core_defaults; 
								break;
							default:
								sm_defaults=sm_macpro_defaults; 
								break;
						}
						break;
					}
					default:
						sm_defaults=sm_macpro_defaults; 
						break;
				}
				break;
			}
		}
	}
	
	for (i=0; sm_defaults[i].key[0]; i++) {
		if (!strcmp (sm_defaults[i].key, key)) {
			return sm_defaults[i].value;
		}
	}

	// Shouldn't happen
	printf ("Error: no default for '%s' known\n", key);
	sleep (2);
	return "";
}

static int sm_get_fsb(const char *name, int table_num)
{
	return Platform->CPU.FSBFrequency/1000000;
}

static int sm_get_cpu (const char *name, int table_num)
{
	return Platform->CPU.CPUFrequency/1000000;
}

static int sm_get_bus_speed (const char *name, int table_num)
{
	if (Platform->CPU.Vendor == 0x756E6547) // Intel
	{		
		switch (Platform->CPU.Family) 
		{
			case 0x06:
			{
				switch (Platform->CPU.Model)
				{
                    case CPUID_MODEL_BANIAS:	// Banias		0x09
                    case CPUID_MODEL_DOTHAN:	// Dothan		0x0D
					case CPUID_MODEL_YONAH:	// Yonah		0x0E
					case CPUID_MODEL_MEROM:	// Merom		0x0F
					case CPUID_MODEL_PENRYN:	// Penryn		0x17
					case CPUID_MODEL_ATOM:	// Atom 45nm	0x1C
						return 0; // TODO: populate bus speed for these processors
						
//					case CPUID_MODEL_FIELDS: // Intel Core i5, i7 LGA1156 (45nm)
//						if (strstr(Platform.CPU.BrandString, "Core(TM) i5"))
//							return 2500; // Core i5
//						return 4800; // Core i7
						
//					case CPUID_MODEL_NEHALEM: // Intel Core i7 LGA1366 (45nm)
//					case CPUID_MODEL_NEHALEM_EX:
//					case CPUID_MODEL_DALES: // Intel Core i5, i7 LGA1156 (45nm) ???
//						return 4800; // GT/s / 1000
//						
					case CPUID_MODEL_WESTMERE_EX: // Intel Core i7 LGA1366 (45nm) 6 Core ???
						return 0; // TODO: populate bus speed for these processors
						
//					case 0x19: // Intel Core i5 650 @3.20 Ghz
//						return 2500; // why? Intel spec says 2.5GT/s 

					case 0x19: // Intel Core i5 650 @3.20 Ghz
					case CPUID_MODEL_NEHALEM: // Intel Core i7 LGA1366 (45nm)
					case CPUID_MODEL_FIELDS: // Intel Core i5, i7 LGA1156 (45nm)
					case CPUID_MODEL_DALES: // Intel Core i5, i7 LGA1156 (45nm) ???
					case CPUID_MODEL_DALES_32NM: // Intel Core i3, i5, i7 LGA1156 (32nm)
                    case CPUID_MODEL_SANDYBRIDGE:
                    case CPUID_MODEL_JAKETOWN:
					case CPUID_MODEL_WESTMERE: // Intel Core i7 LGA1366 (32nm) 6 Core
					case CPUID_MODEL_NEHALEM_EX: // Intel Core i7 LGA1366 (45nm) 6 Core ???
					{ // thanks to dgobe for i3/i5/i7 bus speed detection
						int nhm_bus = 0x3F;
						static long possible_nhm_bus[] = {0xFF, 0x7F, 0x3F};
						unsigned long did, vid;
						int i;
						
						// Nehalem supports Scrubbing
						// First, locate the PCI bus where the MCH is located
						for(i = 0; i < sizeof(possible_nhm_bus); i++)
						{
							vid = pci_config_read16(PCIADDR(possible_nhm_bus[i], 3, 4), 0x00);
							did = pci_config_read16(PCIADDR(possible_nhm_bus[i], 3, 4), 0x02);
							vid &= 0xFFFF;
							did &= 0xFF00;
							
							if(vid == 0x8086 && did >= 0x2C00)
								nhm_bus = possible_nhm_bus[i]; 
						}
						
						unsigned long qpimult, qpibusspeed;
						qpimult = pci_config_read32(PCIADDR(nhm_bus, 2, 1), 0x50);
						qpimult &= 0x7F;
						DBG("qpimult %d\n", qpimult);
						qpibusspeed = (qpimult * 2 * (Platform->CPU.FSBFrequency/1000000));
						// Rek: rounding decimals to match original mac profile info
						if (qpibusspeed%100 != 0)qpibusspeed = ((qpibusspeed+50)/100)*100;
						DBG("qpibusspeed %d\n", qpibusspeed);
						return qpibusspeed;
					}
				}
			}
		}
	}
	return 0;
}

static int sm_get_simplecputype()
{
	if (Platform->CPU.NoCores >= 4) 
	{
		return 0x0501;   // Quad-Core Xeon
	}
	if (((Platform->CPU.NoCores == 1) || (Platform->CPU.NoCores == 2)) && !(platformCPUExtFeature(CPUID_EXTFEATURE_EM64T)))
	{
		return 0x0201;   // Core Solo / Duo
	}
	
	return 0x0301;   // Core 2 Solo / Duo
}

static int sm_get_cputype (const char *name, int table_num)
{
	static bool done = false;		
		
	if (Platform->CPU.Vendor == 0x756E6547) // Intel
	{
		if (!done) {
			verbose("CPU is %s, family 0x%x, model 0x%x\n", Platform->CPU.BrandString, Platform->CPU.Family, Platform->CPU.Model);
			done = true;
		}
		
		switch (Platform->CPU.Family) 
		{
			case 0x06:
			{
				switch (Platform->CPU.Model)
				{
                    case CPUID_MODEL_BANIAS: // Banias
                    case CPUID_MODEL_DOTHAN: // Dothan
					case CPUID_MODEL_YONAH: // Yonah
					case CPUID_MODEL_MEROM: // Merom
					case CPUID_MODEL_PENRYN: // Penryn
					case CPUID_MODEL_ATOM: // Intel Atom (45nm)
						return sm_get_simplecputype();
						
					case CPUID_MODEL_NEHALEM: // Intel Core i7 LGA1366 (45nm)
						return 0x0701; // Core i7
						
					case CPUID_MODEL_FIELDS: // Lynnfield, Clarksfield, Jasper
						if (strstr(Platform->CPU.BrandString, "Core(TM) i5"))
							return 0x601; // Core i5
						return 0x701; // Core i7
						
					case CPUID_MODEL_DALES: // Intel Core i5, i7 LGA1156 (45nm) (Havendale, Auburndale)
						if (strstr(Platform->CPU.BrandString, "Core(TM) i5"))
							return 0x601; // Core i5
						return 0x0701; // Core i7
						
                    case  CPUID_MODEL_SANDYBRIDGE: // Sandybridge
					case CPUID_MODEL_DALES_32NM: // Intel Core i3, i5, i7 LGA1156 (32nm) (Clarkdale, Arrandale)
						if (strstr(Platform->CPU.BrandString, "Core(TM) i3"))
							return 0x901; // Core i3
						if (strstr(Platform->CPU.BrandString, "Core(TM) i5"))
							return 0x601; // Core i5
						if (strstr(Platform->CPU.BrandString, "Core(TM) i7"))							
							return 0x0701; // Core i7 						
						return sm_get_simplecputype();
						
                    case CPUID_MODEL_JAKETOWN:
					case CPUID_MODEL_WESTMERE: // Intel Core i7 LGA1366 (32nm) 6 Core (Gulftown, Westmere-EP, Westmere-WS)
					case CPUID_MODEL_WESTMERE_EX: // Intel Core i7 LGA1366 (45nm) 6 Core ???
						return 0x0701; // Core i7
						
					case 0x19: // Intel Core i5 650 @3.20 Ghz
						return 0x601; // Core i5
				}
			}
		}
	}
	
	return sm_get_simplecputype();
}

static int sm_get_memtype (const char *name, int table_num)
{
	if (execute_hook("isMemoryRegistred", NULL, NULL, NULL, NULL, NULL, NULL) == EFI_SUCCESS) {
	int	map;

	if (table_num < MAX_RAM_SLOTS) {
		map = Platform->DMI.DIMM[table_num];
		if (Platform->RAM.DIMM[map].InUse && Platform->RAM.DIMM[map].Type != 0) {
                    DBG("RAM Detected Type = %d\n", Platform->RAM.DIMM[map].Type);
                    return Platform->RAM.DIMM[map].Type;
		}
	}
	}
	
	return SMB_MEM_TYPE_DDR2;
}

static int sm_get_memspeed (const char *name, int table_num)
{
	if (execute_hook("isMemoryRegistred", NULL, NULL, NULL, NULL, NULL, NULL) == EFI_SUCCESS) {
	int	map;

	if (table_num < MAX_RAM_SLOTS) {
		map = Platform->DMI.DIMM[table_num];
		if (Platform->RAM.DIMM[map].InUse && Platform->RAM.DIMM[map].Frequency != 0) {
                    DBG("RAM Detected Freq = %d Mhz\n", Platform->RAM.DIMM[map].Frequency);
                    return Platform->RAM.DIMM[map].Frequency;
		}
	}
	}
	return 800;
}

static const char *sm_get_memvendor (const char *name, int table_num)
{
	if (execute_hook("isMemoryRegistred", NULL, NULL, NULL, NULL, NULL, NULL) == EFI_SUCCESS) {
	int	map;

	if (table_num < MAX_RAM_SLOTS) {
		map = Platform->DMI.DIMM[table_num];
		if (Platform->RAM.DIMM[map].InUse && strlen(Platform->RAM.DIMM[map].Vendor) > 0) {
			DBG("RAM Detected Vendor[%d]='%s'\n", table_num, Platform->RAM.DIMM[map].Vendor);
			return Platform->RAM.DIMM[map].Vendor;
		}
	}
	}
	return "N/A";
}
	
static const char *sm_get_memserial (const char *name, int table_num)
{
	if (execute_hook("isMemoryRegistred", NULL, NULL, NULL, NULL, NULL, NULL) == EFI_SUCCESS) {
	int	map;

	if (table_num < MAX_RAM_SLOTS) {
		map = Platform->DMI.DIMM[table_num];
		if (Platform->RAM.DIMM[map].InUse && strlen(Platform->RAM.DIMM[map].SerialNo) > 0) {
                   DBG("name = %s, map=%d,  RAM Detected SerialNo[%d]='%s'\n", name ? name : "", 
                        map, table_num, Platform->RAM.DIMM[map].SerialNo);			
                    return Platform->RAM.DIMM[map].SerialNo;
		}
	}
    }
	return "N/A";
}

static const char *sm_get_mempartno (const char *name, int table_num)
{
	if (execute_hook("isMemoryRegistred", NULL, NULL, NULL, NULL, NULL, NULL) == EFI_SUCCESS) {
	int	map;

	if (table_num < MAX_RAM_SLOTS) {
		map = Platform->DMI.DIMM[table_num];
		if (Platform->RAM.DIMM[map].InUse && strlen(Platform->RAM.DIMM[map].PartNo) > 0) {
			DBG("Ram Detected PartNo[%d]='%s'\n", table_num, Platform->RAM.DIMM[map].PartNo);
			return Platform->RAM.DIMM[map].PartNo;
		}
	}
	}
	return "N/A";
}

static int sm_one (int tablen)
{
	return 1;
}

struct smbios_property smbios_properties[]=
{
	{.name="SMbiosvendor",		.table_type= 0,	.value_type=SMSTRING,	.offset=0x04,	.auto_str=sm_get_defstr	},
	{.name="SMbiosversion",		.table_type= 0,	.value_type=SMSTRING,	.offset=0x05,	.auto_str=sm_get_defstr	},
	{.name="SMbiosdate",		.table_type= 0,	.value_type=SMSTRING,	.offset=0x08,	.auto_str=sm_get_defstr	},
	{.name="SMmanufacter",		.table_type= 1,	.value_type=SMSTRING,	.offset=0x04,	.auto_str=sm_get_defstr	},
	{.name="SMproductname",		.table_type= 1,	.value_type=SMSTRING,	.offset=0x05,	.auto_str=sm_get_defstr	},
	{.name="SMsystemversion",	.table_type= 1,	.value_type=SMSTRING,	.offset=0x06,	.auto_str=sm_get_defstr	},
	{.name="SMserial",		.table_type= 1,	.value_type=SMSTRING,	.offset=0x07,	.auto_str=sm_get_defstr	},
	{.name="SMUUID",		.table_type= 1, .value_type=SMOWORD,	.offset=0x08,	.auto_oword=0		},
	{.name="SMfamily",		.table_type= 1,	.value_type=SMSTRING,	.offset=0x1a,	.auto_str=sm_get_defstr	},
	{.name="SMboardmanufacter",	.table_type= 2, .value_type=SMSTRING,	.offset=0x04,	.auto_str=sm_get_defstr	},
	{.name="SMboardproduct",	.table_type= 2, .value_type=SMSTRING,	.offset=0x05,	.auto_str=sm_get_defstr	},
	{.name="SMexternalclock",	.table_type= 4,	.value_type=SMWORD,	.offset=0x12,	.auto_int=sm_get_fsb	},
	{.name="SMmaximalclock",	.table_type= 4,	.value_type=SMWORD,	.offset=0x14,	.auto_int=sm_get_cpu	},
	{.name="SMmemdevloc",		.table_type=17,	.value_type=SMSTRING,	.offset=0x10,	.auto_str=0		},
	{.name="SMmembankloc",		.table_type=17,	.value_type=SMSTRING,	.offset=0x11,	.auto_str=0		},
	{.name="SMmemtype",		.table_type=17,	.value_type=SMBYTE,	.offset=0x12,	.auto_int=sm_get_memtype},
	{.name="SMmemspeed",		.table_type=17,	.value_type=SMWORD,	.offset=0x15,	.auto_int=sm_get_memspeed},
	{.name="SMmemmanufacter",	.table_type=17,	.value_type=SMSTRING,	.offset=0x17,	.auto_str=sm_get_memvendor},
	{.name="SMmemserial",		.table_type=17,	.value_type=SMSTRING,	.offset=0x18,	.auto_str=sm_get_memserial},
	{.name="SMmempart",		.table_type=17,	.value_type=SMSTRING,	.offset=0x1A,	.auto_str=sm_get_mempartno},
	{.name="SMcputype",		.table_type=131,.value_type=SMWORD,	.offset=0x04,	.auto_int=sm_get_cputype},
	{.name="SMbusspeed",		.table_type=132,.value_type=SMWORD,	.offset=0x04,	.auto_int=sm_get_bus_speed}
};

struct smbios_table_description smbios_table_descriptions[]=
{
	{.type=0,	.len=0x18,	.numfunc=sm_one},
	{.type=1,	.len=0x1b,	.numfunc=sm_one},
	{.type=2,	.len=0x0f,	.numfunc=sm_one},
	{.type=4,	.len=0x2a,	.numfunc=sm_one},
	{.type=17,	.len=0x1c,	.numfunc=0},
	{.type=131,	.len=0x06,	.numfunc=sm_one},
	{.type=132,	.len=0x06,	.numfunc=sm_one}
};

/** Compute necessary space requirements for new smbios */
static struct SMBEntryPoint *smbios_dry_run(struct SMBEntryPoint *origsmbios)
{
	struct SMBEntryPoint	*ret;
	char			*smbiostables;
	char			*tablesptr;
	int			origsmbiosnum;
	int			i, j;
	int			tablespresent[256];
	bool			do_auto=true;

	bzero(tablespresent, sizeof(tablespresent));

	getBoolForKey(kSMBIOSdefaults, &do_auto, &bootInfo->bootConfig);

	ret = (struct SMBEntryPoint *)AllocateKernelMemory(sizeof(struct SMBEntryPoint));
	if (origsmbios) {
		smbiostables = (char *)origsmbios->dmi.tableAddress;
		origsmbiosnum = origsmbios->dmi.structureCount;
	} else {
		smbiostables = NULL;
		origsmbiosnum = 0;
	}

	// _SM_
	ret->anchor[0] = 0x5f;
	ret->anchor[1] = 0x53;
	ret->anchor[2] = 0x4d;
	ret->anchor[3] = 0x5f; 
	ret->entryPointLength = sizeof(*ret);
	ret->majorVersion = 2;
	ret->minorVersion = 1;
	ret->maxStructureSize = 0; // will be calculated later in this function
	ret->entryPointRevision = 0;
	for (i=0;i<5;i++) {
		ret->formattedArea[i] = 0;
	}
	//_DMI_
	ret->dmi.anchor[0] = 0x5f;
	ret->dmi.anchor[1] = 0x44;
	ret->dmi.anchor[2] = 0x4d;
	ret->dmi.anchor[3] = 0x49;
	ret->dmi.anchor[4] = 0x5f;
	ret->dmi.tableLength = 0;  // will be calculated later in this function
	ret->dmi.tableAddress = 0; // will be initialized in smbios_real_run()
	ret->dmi.structureCount = 0; // will be calculated later in this function
	ret->dmi.bcdRevision = 0x21;
	tablesptr = smbiostables;

        // add stringlen of overrides to original stringlen, update maxStructure size adequately, 
        // update structure count and tablepresent[type] with count of type. 
	if (smbiostables) {
		for (i=0; i<origsmbiosnum; i++) {
			struct smbios_table_header	*cur = (struct smbios_table_header *)tablesptr;
			char				*stringsptr;
			int				stringlen;

			tablesptr += cur->length;
			stringsptr = tablesptr;
			for (; tablesptr[0]!=0 || tablesptr[1]!=0; tablesptr++);
			tablesptr += 2;
			stringlen = tablesptr - stringsptr - 1;
			if (stringlen == 1) {
				stringlen = 0;
			}
			for (j=0; j<sizeof(smbios_properties)/sizeof(smbios_properties[0]); j++) {
				const char	*str;
				int		size;
				char		altname[40];

				sprintf(altname, "%s_%d",smbios_properties[j].name, tablespresent[cur->type] + 1);				
				if (smbios_properties[j].table_type == cur->type &&
				    smbios_properties[j].value_type == SMSTRING &&
				    (getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig) ||
				     getValueForKey(altname,&str, &size, &bootInfo->smbiosConfig)))
				{
					stringlen += size + 1;
				} else if (smbios_properties[j].table_type == cur->type &&
				           smbios_properties[j].value_type == SMSTRING &&
				           do_auto && smbios_properties[j].auto_str)
				{
					stringlen += strlen(smbios_properties[j].auto_str(smbios_properties[j].name, tablespresent[cur->type])) + 1;
				}
			}
			if (stringlen == 0) {
				stringlen = 1;
			}
			stringlen++;
			if (ret->maxStructureSize < cur->length+stringlen) {
				ret->maxStructureSize=cur->length+stringlen;
			}
			ret->dmi.tableLength += cur->length+stringlen;
			ret->dmi.structureCount++;
			tablespresent[cur->type]++;
		}
	}
        // Add eventually table types whose detected count would be < required count, and update ret header with:
        // new stringlen addons, structure count, and tablepresent[type] count adequately
	for (i=0; i<sizeof(smbios_table_descriptions)/sizeof(smbios_table_descriptions[0]); i++) {
		int	numnec=-1;
		char	buffer[40];

		sprintf(buffer, "SMtable%d", i);
		if (!getIntForKey(buffer, &numnec, &bootInfo->smbiosConfig)) {
			numnec = -1;
		}
		if (numnec==-1 && do_auto && smbios_table_descriptions[i].numfunc) {
			numnec = smbios_table_descriptions[i].numfunc(smbios_table_descriptions[i].type);
		}
		while (tablespresent[smbios_table_descriptions[i].type] < numnec) {
			int	stringlen = 0;
			for (j=0; j<sizeof(smbios_properties)/sizeof(smbios_properties[0]); j++) {
				const char	*str;
				int		size;
				char		altname[40];

				sprintf(altname, "%s_%d",smbios_properties[j].name, tablespresent[smbios_table_descriptions[i].type] + 1);
				if (smbios_properties[j].table_type == smbios_table_descriptions[i].type &&
				    smbios_properties[j].value_type == SMSTRING &&
				    (getValueForKey(altname, &str, &size, &bootInfo->smbiosConfig) ||
				     getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig)))
				{
					stringlen += size + 1;
				} else if (smbios_properties[j].table_type == smbios_table_descriptions[i].type &&
				           smbios_properties[j].value_type==SMSTRING &&
				           do_auto && smbios_properties[j].auto_str)
				{
					stringlen += strlen(smbios_properties[j].auto_str(smbios_properties[j].name, tablespresent[smbios_table_descriptions[i].type])) + 1;
				}
			}
			if (stringlen == 0) {
				stringlen = 1;
			}
			stringlen++;
			if (ret->maxStructureSize < smbios_table_descriptions[i].len+stringlen) {
				ret->maxStructureSize = smbios_table_descriptions[i].len + stringlen;
			}
			ret->dmi.tableLength += smbios_table_descriptions[i].len + stringlen;
			ret->dmi.structureCount++;
			tablespresent[smbios_table_descriptions[i].type]++;
		}
	}
	return ret;
}

/** From the origsmbios detected by getAddressOfSmbiosTable() to newsmbios whose entrypoint 
 * struct has been created by smbios_dry_run, update each table struct content of new smbios
 * int the new allocated table address of size newsmbios->tablelength.
 */
static void smbios_real_run(struct SMBEntryPoint * origsmbios, struct SMBEntryPoint * newsmbios)
{
	char *smbiostables;
	char *tablesptr, *newtablesptr;
	int origsmbiosnum;
	// bitmask of used handles
	uint8_t handles[8192]; 
	uint16_t nexthandle=0;
	int i, j;
	int tablespresent[256];
	bool do_auto=true;
	
    static bool done = false; // IMPROVEME: called twice via getSmbios(), but only the second call can get all necessary info !

	bzero(tablespresent, sizeof(tablespresent));
	bzero(handles, sizeof(handles));

	getBoolForKey(kSMBIOSdefaults, &do_auto, &bootInfo->bootConfig);
	
	newsmbios->dmi.tableAddress = (uint32_t)AllocateKernelMemory(newsmbios->dmi.tableLength);
	if (origsmbios) {
		smbiostables = (char *)origsmbios->dmi.tableAddress;
		origsmbiosnum = origsmbios->dmi.structureCount;
	} else {
		smbiostables = NULL;
		origsmbiosnum = 0;
	}
	tablesptr = smbiostables;
	newtablesptr = (char *)newsmbios->dmi.tableAddress;

        // if old smbios exists then update new smbios  with old smbios original content first
	if (smbiostables) {
		for (i=0; i<origsmbiosnum; i++) {
			struct smbios_table_header	*oldcur = (struct smbios_table_header *) tablesptr;
			struct smbios_table_header	*newcur = (struct smbios_table_header *) newtablesptr;
			char				*stringsptr;
			int				nstrings = 0;

			handles[(oldcur->handle) / 8] |= 1 << ((oldcur->handle) % 8);

                        // copy table length from old table to new table but not the old strings
			memcpy(newcur,oldcur, oldcur->length);

			tablesptr += oldcur->length;
			stringsptr = tablesptr;
			newtablesptr += oldcur->length;

                        // calculate the number of strings in the old content
			for (;tablesptr[0]!=0 || tablesptr[1]!=0; tablesptr++) {
				if (tablesptr[0] == 0) {
					nstrings++;
				}
			}
			if (tablesptr != stringsptr) {
				nstrings++;
			}
			tablesptr += 2;

                        // copy the old strings to new table
			memcpy(newtablesptr, stringsptr, tablesptr-stringsptr);

 			// point to next possible space for a string (deducting the second 0 char at the end)
			newtablesptr += tablesptr - stringsptr - 1;
                            if (nstrings == 0) { // if no string was found rewind to the first 0 char of the 0,0 terminator
				newtablesptr--;
			}

                        // now for each property in the table update the overrides if any (auto or user)
			for (j=0; j<sizeof(smbios_properties)/sizeof(smbios_properties[0]); j++) {
				const char	*str;
				int		size;
				int		num;
				char		altname[40];

				sprintf(altname, "%s_%d", smbios_properties[j].name, tablespresent[newcur->type] + 1);
				if (smbios_properties[j].table_type == newcur->type) {
					switch (smbios_properties[j].value_type) {
					case SMSTRING:
						if (getValueForKey(altname, &str, &size, &bootInfo->smbiosConfig) ||
						    getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig))
						{
							memcpy(newtablesptr, str, size);
							newtablesptr[size] = 0;
							newtablesptr += size + 1;
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = ++nstrings;
						} else if (do_auto && smbios_properties[j].auto_str) {
							str = smbios_properties[j].auto_str(smbios_properties[j].name, tablespresent[newcur->type]);
							size = strlen(str);
							memcpy(newtablesptr, str, size);
							newtablesptr[size] = 0;
							newtablesptr += size + 1;
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = ++nstrings;
						}
						break;

					case SMOWORD:
						if (getValueForKey(altname, &str, &size, &bootInfo->smbiosConfig) ||
						    getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig))
						{
							int		k=0, t=0, kk=0;
							const char	*ptr = str;
							memset(((char*)newcur) + smbios_properties[j].offset, 0, 16);
							while (ptr-str<size && *ptr && (*ptr==' ' || *ptr=='\t' || *ptr=='\n')) {
								ptr++;
							}
							if (size-(ptr-str)>=2 && ptr[0]=='0' && (ptr[1]=='x' || ptr[1]=='X')) {
								ptr += 2;
							}
							for (;ptr-str<size && *ptr && k<16;ptr++) {
								if (*ptr>='0' && *ptr<='9') {
									(t=(t<<4)|(*ptr-'0')),kk++;
								}
								if (*ptr>='a' && *ptr<='f') {
									(t=(t<<4)|(*ptr-'a'+10)),kk++;
								}
								if (*ptr>='A' && *ptr<='F') {
									(t=(t<<4)|(*ptr-'A'+10)),kk++;
								}
								if (kk == 2) {
									*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset + k)) = t;
									k++;
									kk = 0;
									t = 0;
								}
							}
						}
						break;

					case SMBYTE:
						if (getIntForKey(altname, &num, &bootInfo->smbiosConfig) ||
						    getIntForKey(smbios_properties[j].name, &num, &bootInfo->smbiosConfig))
						{
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = num;
						} else if (do_auto && smbios_properties[j].auto_int) {
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = smbios_properties[j].auto_int(smbios_properties[j].name, tablespresent[newcur->type]);							
						}
						break;

					case SMWORD:
						if (getIntForKey(altname, &num, &bootInfo->smbiosConfig) ||
						    getIntForKey(smbios_properties[j].name, &num, &bootInfo->smbiosConfig))
						{
							*((uint16_t*)(((char*)newcur) + smbios_properties[j].offset)) = num;
						} else if (do_auto && smbios_properties[j].auto_int) {
							*((uint16_t*)(((char*)newcur) + smbios_properties[j].offset)) = smbios_properties[j].auto_int(smbios_properties[j].name, tablespresent[newcur->type]);
						}
						break;
					}
				}
			}
			if (nstrings == 0) {
				newtablesptr[0] = 0;
				newtablesptr++;
			}
			newtablesptr[0] = 0;
			newtablesptr++;
			tablespresent[newcur->type]++;
		}
	}

        // for each eventual complementary table not present in the original smbios, do the overrides
	for (i=0; i<sizeof(smbios_table_descriptions)/sizeof(smbios_table_descriptions[0]); i++) {
		int	numnec = -1;
		char	buffer[40];

		sprintf(buffer, "SMtable%d", i);
		if (!getIntForKey(buffer, &numnec, &bootInfo->smbiosConfig)) {
			numnec = -1;
		}
		if (numnec == -1 && do_auto && smbios_table_descriptions[i].numfunc) {
			numnec = smbios_table_descriptions[i].numfunc(smbios_table_descriptions[i].type);
		}
		while (tablespresent[smbios_table_descriptions[i].type] < numnec) {
			struct smbios_table_header	*newcur = (struct smbios_table_header *) newtablesptr;
			int				nstrings = 0;

			memset(newcur,0, smbios_table_descriptions[i].len);
			while (handles[(nexthandle)/8] & (1 << ((nexthandle) % 8))) {
				nexthandle++;
			}
			newcur->handle = nexthandle;
			handles[nexthandle / 8] |= 1 << (nexthandle % 8);
			newcur->type = smbios_table_descriptions[i].type;
			newcur->length = smbios_table_descriptions[i].len;
			newtablesptr += smbios_table_descriptions[i].len;
			for (j=0; j<sizeof(smbios_properties)/sizeof(smbios_properties[0]); j++) {
				const char	*str;
				int		size;
				int		num;
				char		altname[40];

				sprintf(altname, "%s_%d", smbios_properties[j].name, tablespresent[newcur->type] + 1);
				if (smbios_properties[j].table_type == newcur->type) {
					switch (smbios_properties[j].value_type) {
					case SMSTRING:
						if (getValueForKey(altname, &str, &size, &bootInfo->smbiosConfig) ||
						    getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig))
						{
							memcpy(newtablesptr, str, size);
							newtablesptr[size] = 0;
							newtablesptr += size + 1;
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = ++nstrings;
						} else if (do_auto && smbios_properties[j].auto_str) {
							str = smbios_properties[j].auto_str(smbios_properties[j].name, tablespresent[newcur->type]);
							size = strlen(str);
							memcpy(newtablesptr, str, size);
							newtablesptr[size] = 0;
							newtablesptr += size + 1;
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = ++nstrings;
						}
						break;

					case SMOWORD:
						if (getValueForKey(altname, &str, &size, &bootInfo->smbiosConfig) ||
						    getValueForKey(smbios_properties[j].name, &str, &size, &bootInfo->smbiosConfig))
						{
							int		k=0, t=0, kk=0;
							const char	*ptr = str;

							memset(((char*)newcur) + smbios_properties[j].offset, 0, 16);
							while (ptr-str<size && *ptr && (*ptr==' ' || *ptr=='\t' || *ptr=='\n')) {
								ptr++;
							}
							if (size-(ptr-str)>=2 && ptr[0]=='0' && (ptr[1]=='x' || ptr[1]=='X')) {
								ptr += 2;
							}
							for (;ptr-str<size && *ptr && k<16;ptr++) {
								if (*ptr>='0' && *ptr<='9') {
									(t=(t<<4)|(*ptr-'0')),kk++;
								}
								if (*ptr>='a' && *ptr<='f') {
									(t=(t<<4)|(*ptr-'a'+10)),kk++;
								}
								if (*ptr>='A' && *ptr<='F') {
									(t=(t<<4)|(*ptr-'A'+10)),kk++;
								}
								if (kk == 2) {
									*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset + k)) = t;
									k++;
									kk = 0;
									t = 0;
								}
							}
						}
						break;
						
					case SMBYTE:
						if (getIntForKey(altname, &num, &bootInfo->smbiosConfig) ||
						    getIntForKey(smbios_properties[j].name, &num, &bootInfo->smbiosConfig))
						{
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = num;
						} else if (do_auto && smbios_properties[j].auto_int) {
							*((uint8_t*)(((char*)newcur) + smbios_properties[j].offset)) = smbios_properties[j].auto_int(smbios_properties[j].name, tablespresent[newcur->type]);
						}
						break;
						
					case SMWORD:
						if (getIntForKey(altname, &num, &bootInfo->smbiosConfig) ||
						    getIntForKey(smbios_properties[j].name, &num, &bootInfo->smbiosConfig))
						{
							*((uint16_t*)(((char*)newcur) + smbios_properties[j].offset)) = num;
						} else if (do_auto && smbios_properties[j].auto_int) {
							*((uint16_t*)(((char*)newcur)+smbios_properties[j].offset)) = smbios_properties[j].auto_int(smbios_properties[j].name, tablespresent[newcur->type]);
						}
						break;
					}
				}
			}
			if (nstrings == 0) {
				newtablesptr[0] = 0;
				newtablesptr++;
			}
			newtablesptr[0] = 0;
			newtablesptr++;
			tablespresent[smbios_table_descriptions[i].type]++;
		}
	}

        // calculate new checksums
	newsmbios->dmi.checksum = 0;
	newsmbios->dmi.checksum = 256 - checksum8(&newsmbios->dmi, sizeof(newsmbios->dmi));
	newsmbios->checksum = 0;
	newsmbios->checksum = 256 - checksum8(newsmbios, sizeof(*newsmbios));
	
	if (!done) {
		verbose("Patched DMI Table\n");
		done=true;
	}
}

#define MAX_DMI_TABLES 96
typedef struct DmiNumAssocTag {
    struct DMIHeader * dmi;
    uint8_t type;
} DmiNumAssoc;

static DmiNumAssoc DmiTablePair[MAX_DMI_TABLES];
static int DmiTablePairCount = 0;
static int current_pos=0;
static bool ftTablePairInit = true;

/** 
 * Get a table structure entry from a type specification and a smbios address
 * return NULL if table is not found
 */
void getSmbiosTableStructure(struct SMBEntryPoint *smbios)
{
    struct DMIHeader * dmihdr=NULL;
    SMBByte* p;
    int i;
	
    if (ftTablePairInit && smbios!=NULL) {
        ftTablePairInit = false;
#if DEBUG_SMBIOS
        printf(">>> SMBIOSAddr=0x%08x\n", smbios);
        printf(">>> DMI: addr=0x%08x, len=%d, count=%d\n", smbios->dmi.tableAddress, 
               smbios->dmi.tableLength, smbios->dmi.structureCount);
#endif
        p = (SMBByte *) smbios->dmi.tableAddress;
        for (i=0; 
             i < smbios->dmi.structureCount && 
             p + 4 <= (SMBByte *)smbios->dmi.tableAddress + smbios->dmi.tableLength; 
             i++)   {
            dmihdr = (struct DMIHeader *) p;
			
#if DEBUG_SMBIOS
            // verbose(">>>>>> DMI(%d): type=0x%02x, len=0x%d\n",i,dmihdr->type,dmihdr->length);
#endif
            if (dmihdr->length < 4 || dmihdr->type == 127 /* EOT */) break;
            if (DmiTablePairCount < MAX_DMI_TABLES) {
                DmiTablePair[DmiTablePairCount].dmi = dmihdr;
                DmiTablePair[DmiTablePairCount].type = dmihdr->type;
                DmiTablePairCount++;
            }
            else {
                printf("DMI table entries list is full! Next entries won't be stored.\n");
            }
#if DEBUG_SMBIOS
            printf("DMI header found for table type %d, length = %d\n", dmihdr->type, dmihdr->length);
#endif
            p = p + dmihdr->length;
            while ((p - (SMBByte *)smbios->dmi.tableAddress + 1 < smbios->dmi.tableLength) && (p[0] != 0x00 || p[1] != 0x00))  {
                p++;
			}
            p += 2;
		}
        
    }
}

/** Find first original dmi Table with a particular type */
struct DMIHeader* FindFirstDmiTableOfType(int type, int minlength)
{
    current_pos = 0;
    
    return FindNextDmiTableOfType(type, minlength);
};

/** Find next original dmi Table with a particular type */
struct DMIHeader* FindNextDmiTableOfType(int type, int minlength)
{
    int i;
	
    if (ftTablePairInit) getSmbiosOriginal();
	
    for (i=current_pos; i < DmiTablePairCount; i++) {
        if (type == DmiTablePair[i].type && 
            DmiTablePair[i].dmi &&
            DmiTablePair[i].dmi->length >= minlength ) {
            current_pos = i+1;
            return DmiTablePair[i].dmi;
        }
    }
    return NULL; // not found
};


const char * smbiosStringAtIndex(DMIHeader* smHeader, int index, int* length )
{
    const char * last = 0;
    const char * next = (const char *) smHeader + smHeader->length;
	
    if ( length ) *length = 0;
    while ( index-- )
    {
        last = 0;
		const char * cp = 0;
		for ( cp = next; *cp || cp[1]; cp++ )
        {
            if ( *cp == '\0' )
            {
                last = next;
                next = cp + 1;
                break;
            }
        }
        if ( last == 0 ) break;
    }
	
    if ( last )
    {
        while (*last == ' ') last++;
        if (length)
        {
            UInt8 len;
            for ( len = next - last - 1; len && last[len - 1] == ' '; len-- )
                ;
            *length = len; // number of chars not counting the terminating NULL
        }
    }
	
    return last ? last : "";
}


struct SMBEntryPoint *getSmbiosPatched(struct SMBEntryPoint *orig)
{       
   	struct SMBEntryPoint *patched = NULL; // cached
            
	patched = smbios_dry_run(orig);
            if(patched==NULL) {
                printf("Could not create new SMBIOS !!\n");
                pause();
            }
            else {
                smbios_real_run(orig, patched);
            }
        

       return patched;
   
}

/*
char* getSmbiosProductName()
{
	struct SMBEntryPoint	*smbios;
	SMBSystemInformation	*p;
	char*					tempString;
	int						tmpLen;
	
	smbios = getSmbiosOriginal();
	if (smbios==NULL) return NULL; 
	
	p = (SMBSystemInformation*) FindFirstDmiTableOfType(1, 0x19); // Type 1: (3.3.2) System Information
	if (p==NULL) return NULL;
	
	
	tempString = (char*)smbiosStringAtIndex((DMIHeader*)p, p->productName, &tmpLen);
	tempString[tmpLen] = 0;
	
	gSMBIOSBoardModel = malloc(tmpLen + 1);
	if(gSMBIOSBoardModel)
	{
		strncpy(gSMBIOSBoardModel, tempString, tmpLen);
		Node* node = DT__FindNode("/", false);
		DT__AddProperty(node, "orig-model", tmpLen, gSMBIOSBoardModel);
	}
	verbose("Actual model name is '%s'\n", tempString);
	return tempString;
}
*/

void scan_memory(PlatformInfo_t *p)
{		 
    int i=0;
    struct DMIHeader * dmihdr = NULL;
    
    struct DMIMemoryModuleInfo* memInfo[MAX_RAM_SLOTS]; // 6
    struct DMIPhysicalMemoryArray* physMemArray; // 16
    struct DMIMemoryDevice* memDev[MAX_RAM_SLOTS]; //17
	
    /* We mainly don't use obsolete tables 5,6 because most of computers don't handle it anymore */
	Platform->DMI.MemoryModules = 0;
    /* Now lets peek info rom table 16,17 as for some bios, table 5 & 6 are not used */
    physMemArray = (struct DMIPhysicalMemoryArray*) FindFirstDmiTableOfType(16, 4);
    Platform->DMI.MaxMemorySlots = physMemArray ? physMemArray->numberOfMemoryDevices :  0;
	
    i = 0;
    for(dmihdr = FindFirstDmiTableOfType(17, 4);
        dmihdr;
        dmihdr = FindNextDmiTableOfType(17, 4) ) {
        memDev[i] = (struct DMIMemoryDevice*) dmihdr;
        if (memDev[i]->size !=0 ) Platform->DMI.MemoryModules++;
        if (memDev[i]->speed>0) Platform->RAM.DIMM[i].Frequency = memDev[i]->speed; // take it here for now but we'll check spd and dmi table 6 as well
        i++;
    }
    // for table 6, we only have a look at the current speed
    i = 0;
    for(dmihdr = FindFirstDmiTableOfType(6, 4);
        dmihdr;
        dmihdr = FindNextDmiTableOfType(6, 4) ) {
        memInfo[i] = (struct DMIMemoryModuleInfo*) dmihdr;
        if (memInfo[i]->currentSpeed > Platform->RAM.DIMM[i].Frequency) 
            Platform->RAM.DIMM[i].Frequency = memInfo[i]->currentSpeed; // favor real overclocked speed if any
        i++;
    }
#if 0
    dumpAllTablesOfType(17);
    getc();
#endif
}