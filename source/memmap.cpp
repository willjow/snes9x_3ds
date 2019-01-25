#include "copyright.h"


#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>

#ifdef __linux
#include <unistd.h>
#endif

#include "snes9x.h"
#include "memmap.h"
#include "cpuexec.h"
#include "ppu.h"
#include "cheats.h"
#include "apu.h"
#include "sa1.h"
#include "dsp.h"
#include "srtc.h"
#include "sdd1.h"
#include "spc7110.h"
#include "seta.h"
#include "bsx.h"

#include "3dsimpl.h"

#ifdef __W32_HEAP
#include <malloc.h>
#endif

#ifndef ZSNES_FX
#include "fxemu.h"
extern struct FxInit_s SuperFX;
#else
START_EXTERN_C
extern uint8 *SFXPlotTable;
END_EXTERN_C
#endif

#ifndef SET_UI_COLOR
#define SET_UI_COLOR(r,g,b) ;
#endif

//you would think everyone would have these
//since they're so useful.
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static int retry_count=0;
static uint8 bytes0x2000 [0x2000];
int is_bsx(unsigned char *);
int bs_name(unsigned char *);
int check_char(unsigned);
void S9xDeinterleaveType2 (bool8 reset=TRUE);
inline uint32 caCRC32(uint8 *array, uint32 size, register uint32 crc32 = 0xFFFFFFFF);

extern char *rom_filename;

const uint32 crc32Table[256] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



void S9xDeinterleaveType1(int TotalFileSize, uint8 * base)
{
	int i;
	int nblocks = TotalFileSize >> 16;
	uint8 blocks [256];
	for (i = 0; i < nblocks; i++)
	{
		blocks [i * 2] = i + nblocks;
		blocks [i * 2 + 1] = i;
	}
	uint8 *tmp = (uint8 *) malloc (0x8000);
	if (tmp)
	{
		for (i = 0; i < nblocks * 2; i++)
		{
			for (int j = i; j < nblocks * 2; j++)
			{
				if (blocks [j] == i)
				{
					memmove (tmp, &base [blocks [j] * 0x8000], 0x8000);
					memmove (&base [blocks [j] * 0x8000], 
						&base [blocks [i] * 0x8000], 0x8000);
					memmove (&base [blocks [i] * 0x8000], tmp, 0x8000);
					uint8 b = blocks [j];
					blocks [j] = blocks [i];
					blocks [i] = b;
					break;
				}
			}
		}
		free ((char *) tmp);
	}
}

void S9xDeinterleaveGD24(int TotalFileSize, uint8 * base)
{

	if(TotalFileSize!=0x300000)
		return;

	uint8 *tmp = (uint8 *) malloc (0x80000);
	if (tmp)
	{
		memmove(tmp, &base[0x180000], 0x80000);
		memmove(&base[0x180000], &base[0x200000], 0x80000);
		memmove(&base[0x200000], &base[0x280000], 0x80000);
		memmove(&base[0x280000], tmp, 0x80000);
		free ((char *) tmp);

		S9xDeinterleaveType1(TotalFileSize, base);
	}
}

bool8 CMemory::AllASCII (uint8 *b, int size)
{
    for (int i = 0; i < size; i++)
    {
		if (b[i] < 32 || b[i] > 126)
			return (FALSE);
    }
    return (TRUE);
}

int CMemory::ScoreHiROM (bool8 skip_header, int32 romoff)
{
    int score = 0;
    int o = skip_header ? 0xff00 + 0x200 : 0xff00;
	
	o+=romoff;

	if(Memory.ROM [o + 0xd5] & 0x1)
		score+=2;

	//Mode23 is SA-1
	if(Memory.ROM [o + 0xd5] == 0x23)
		score-=2;

	if(Memory.ROM [o+0xd4] == 0x20)
		score +=2;

    if ((Memory.ROM [o + 0xdc] + (Memory.ROM [o + 0xdd] << 8) +
		Memory.ROM [o + 0xde] + (Memory.ROM [o + 0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if(0!=(Memory.ROM [o + 0xde] + (Memory.ROM [o + 0xdf] << 8)))
			score++;
	}
	
    if (Memory.ROM [o + 0xda] == 0x33)
		score += 2;
    if ((Memory.ROM [o + 0xd5] & 0xf) < 4)
		score += 2;
    if (!(Memory.ROM [o + 0xfd] & 0x80))
		score -= 6;
    if ((Memory.ROM [o + 0xfc]|(Memory.ROM [o + 0xfd]<<8))>0xFFB0)
		score -= 2; //reduced after looking at a scan by Cowering
    if (CalculatedSize > 1024 * 1024 * 3)
		score += 4;
    if ((1 << (Memory.ROM [o + 0xd7] - 7)) > 48)
		score -= 1;
    if (!AllASCII (&Memory.ROM [o + 0xb0], 6))
		score -= 1;
    if (!AllASCII (&Memory.ROM [o + 0xc0], ROM_NAME_LEN - 1))
		score -= 1;
	
    return (score);
}	

int CMemory::ScoreLoROM (bool8 skip_header, int32 romoff)
{
    int score = 0;
    int o = skip_header ? 0x7f00 + 0x200 : 0x7f00;
	
	o+=romoff;

	if(!(Memory.ROM [o + 0xd5] & 0x1))
		score+=3;

	//Mode23 is SA-1
	if(Memory.ROM [o + 0xd5] == 0x23)
		score+=2;

    if ((Memory.ROM [o + 0xdc] + (Memory.ROM [o + 0xdd] << 8) +
		Memory.ROM [o + 0xde] + (Memory.ROM [o + 0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if(0!=(Memory.ROM [o + 0xde] + (Memory.ROM [o + 0xdf] << 8)))
			score++;
	}
	
    if (Memory.ROM [o + 0xda] == 0x33)
		score += 2;
    if ((Memory.ROM [o + 0xd5] & 0xf) < 4)
		score += 2;
    if (CalculatedSize <= 1024 * 1024 * 16)
		score += 2;
    if (!(Memory.ROM [o + 0xfd] & 0x80))
		score -= 6;
	if ((Memory.ROM [o + 0xfc]|(Memory.ROM [o + 0xfd]<<8))>0xFFB0)
		score -= 2;//reduced per Cowering suggestion
    if ((1 << (Memory.ROM [o + 0xd7] - 7)) > 48)
		score -= 1;
    if (!AllASCII (&Memory.ROM [o + 0xb0], 6))
		score -= 1;
    if (!AllASCII (&Memory.ROM [o + 0xc0], ROM_NAME_LEN - 1))
		score -= 1;
	
    return (score);
}

char *CMemory::Safe (const char *s)
{
    static char *safe;
    static int safe_len = 0;

	if(s==NULL)
	{
		if(safe!=NULL)
		{
			free((char*)safe);
			safe = NULL;
		}
		return NULL;
	}
    int len = strlen (s);
    if (!safe || len + 1 > safe_len)
    {
		if (safe)
			free ((char *) safe);
		safe = (char *) malloc (safe_len = len + 1);
    }
	
    for (int i = 0; i < len; i++)
    {
		if (s [i] >= 32 && s [i] < 127)
			safe [i] = s[i];
		else
			safe [i] = '?';
    }
    safe [len] = 0;
    return (safe);
}

/**********************************************************************************************/
/* Init()                                                                                     */
/* This function allocates all the memory needed by the emulator                              */
/**********************************************************************************************/
bool8 CMemory::Init ()
{
    RAM	    = (uint8 *) malloc (0x20000);
    SRAM    = (uint8 *) malloc (0x20000);
    VRAM    = (uint8 *) malloc (0x10000);
    ROM     = (uint8 *) malloc (MAX_ROM_SIZE + 0x200 + 0x8000);
	memset (RAM, 0, 0x20000);
	memset (SRAM, 0, 0x20000);
	memset (VRAM, 0, 0x10000);
	memset (ROM, 0, MAX_ROM_SIZE + 0x200 + 0x8000);
    
	// From Snes9x v1.52
	//BSRAM	= (uint8 *) malloc (0x80000);
	//memset (BSRAM, 0, 0x80000);
	BIOSROM = ROM + 0x300000; // BS
	BSRAM   = ROM + 0x400000; // BS

	FillRAM = NULL;
	
    IPPU.TileCache [TILE_2BIT] = (uint8 *) malloc (MAX_2BIT_TILES * 128);
    IPPU.TileCache [TILE_4BIT] = (uint8 *) malloc (MAX_4BIT_TILES * 128);
    IPPU.TileCache [TILE_8BIT] = (uint8 *) malloc (MAX_8BIT_TILES * 128);
    
    IPPU.TileCached [TILE_2BIT] = (uint8 *) malloc (MAX_2BIT_TILES);
    IPPU.TileCached [TILE_4BIT] = (uint8 *) malloc (MAX_4BIT_TILES);
    IPPU.TileCached [TILE_8BIT] = (uint8 *) malloc (MAX_8BIT_TILES);
    
    if (!RAM || !SRAM || !VRAM || !ROM || !BSRAM ||
        !IPPU.TileCache [TILE_2BIT] || !IPPU.TileCache [TILE_4BIT] ||
		!IPPU.TileCache [TILE_8BIT] || !IPPU.TileCached [TILE_2BIT] ||
		!IPPU.TileCached [TILE_4BIT] ||	!IPPU.TileCached [TILE_8BIT])
    {
		Deinit ();
		return (FALSE);
    }
	
    // FillRAM uses first 32K of ROM image area, otherwise space just
    // wasted. Might be read by the SuperFX code.
	
    FillRAM = ROM;
	
    // Add 0x8000 to ROM image pointer to stop SuperFX code accessing
    // unallocated memory (can cause crash on some ports).
    ROM += 0x8000;
	
    C4RAM    = ROM + 0x400000 + 8192 * 8;
    ::ROM    = ROM;
    ::SRAM   = SRAM;
    ::RegRAM = FillRAM;
	
    SuperFX.pvRegisters = &Memory.FillRAM [0x3000];
    SuperFX.nRamBanks = 2;	// Most only use 1.  1=64KB, 2=128KB=1024Mb
    SuperFX.pvRam = ::SRAM;
    SuperFX.nRomBanks = (2 * 1024 * 1024) / (32 * 1024);
    SuperFX.pvRom = (uint8 *) ROM;
	
    ZeroMemory (IPPU.TileCache [TILE_2BIT], MAX_2BIT_TILES * 128);
    ZeroMemory (IPPU.TileCache [TILE_4BIT], MAX_4BIT_TILES * 128);
    ZeroMemory (IPPU.TileCache [TILE_8BIT], MAX_8BIT_TILES * 128);

    ZeroMemory (IPPU.TileCached [TILE_2BIT], MAX_2BIT_TILES);
    ZeroMemory (IPPU.TileCached [TILE_4BIT], MAX_4BIT_TILES);
    ZeroMemory (IPPU.TileCached [TILE_8BIT], MAX_8BIT_TILES);
    
 //   SDD1Data = NULL;
 //   SDD1Index = NULL;
	
	// Optimizations: Save a copy of the memory map address in the CPU global variable
	CPU.MemoryMap = this->Map;
	CPU.MemoryWriteMap = this->WriteMap;
	CPU.MemorySpeed = this->MemorySpeed;
	
    return (TRUE);
}

void CMemory::Deinit ()
{
#ifdef __W32_HEAP
	if(_HEAPOK!=_heapchk())
		MessageBox(GUI.hWnd, "CMemory::Deinit", "Heap Corrupt", MB_OK);
#endif

    if (RAM)
    {
		free ((char *) RAM);
		RAM = NULL;
    }
    if (SRAM)
    {
		free ((char *) SRAM);
		SRAM = NULL;
    }
    if (VRAM)
    {
		free ((char *) VRAM);
		VRAM = NULL;
    }
    if (ROM)
    {
		ROM -= 0x8000;
		free ((char *) ROM);
		ROM = NULL;
    }
	
	/*if(BSRAM)
	{
		free((char*) BSRAM);
		BSRAM=NULL;
	}*/

    if (IPPU.TileCache [TILE_2BIT])
    {
		free ((char *) IPPU.TileCache [TILE_2BIT]);
		IPPU.TileCache [TILE_2BIT] = NULL;
    }
    if (IPPU.TileCache [TILE_4BIT])
    {
		free ((char *) IPPU.TileCache [TILE_4BIT]);
		IPPU.TileCache [TILE_4BIT] = NULL;
    }
    if (IPPU.TileCache [TILE_8BIT])
    {
		free ((char *) IPPU.TileCache [TILE_8BIT]);
		IPPU.TileCache [TILE_8BIT] = NULL;
    }
	
    if (IPPU.TileCached [TILE_2BIT])
    {
		free ((char *) IPPU.TileCached [TILE_2BIT]);
		IPPU.TileCached [TILE_2BIT] = NULL;
    }
    if (IPPU.TileCached [TILE_4BIT])
    {
		free ((char *) IPPU.TileCached [TILE_4BIT]);
		IPPU.TileCached [TILE_4BIT] = NULL;
    }
    if (IPPU.TileCached [TILE_8BIT])
    {
		free ((char *) IPPU.TileCached [TILE_8BIT]);
		IPPU.TileCached [TILE_8BIT] = NULL;
    }
    //FreeSDD1Data ();
	Safe(NULL);
}
/*
void CMemory::FreeSDD1Data ()
{
    if (SDD1Index)
    {
		free ((char *) SDD1Index);
		SDD1Index = NULL;
    }
    if (SDD1Data)
    {
		free ((char *) SDD1Data);
		SDD1Data = NULL;
    }
}
*/
/**********************************************************************************************/
/* LoadROM()                                                                                  */
/* This function loads a Snes-Backup image                                                    */
/**********************************************************************************************/

bool8 CMemory::LoadROM (const char *filename)
{
    int32 TotalFileSize = 0;
    bool8 Interleaved = FALSE;
    bool8 Tales = FALSE;
 
	uint8* RomHeader=ROM;
	
	ExtendedFormat=NOPE;

 	if(CleanUp7110!=NULL)
		(*CleanUp7110)();
	
    memset (&SNESGameFixes, 0, sizeof(SNESGameFixes));
    SNESGameFixes.SRAMInitialValue = 0x60;
	
    memset (bytes0x2000, 0, 0x2000);
    CPU.TriedInterleavedMode2 = FALSE;
	
    CalculatedSize = 0;
	retry_count =0;

again:

	TotalFileSize = FileLoader(ROM, filename, MAX_ROM_SIZE);

	if (!TotalFileSize)
		return FALSE;		// it ends here
	else if(!Settings.NoPatch)
		CheckForIPSPatch (filename, HeaderCount != 0, TotalFileSize);

	//fix hacked games here.
	if((strncmp("HONKAKUHA IGO GOSEI", (char*)&ROM[0x7FC0],19)==0)&&(ROM[0x7FD5]!=0x31))
	{
		ROM[0x7FD5]=0x31;
		ROM[0x7FD6]=0x02;
		S9xMessage(S9X_ERROR,S9X_ROM_CONFUSING_FORMAT_INFO, "Warning! Hacked Dump!");
	}

	if((strncmp("HONKAKUHA IGO GOSEI", (char*)&ROM[0xFFC0],19)==0)&&(ROM[0xFFD5]!=0x31))
	{
		ROM[0xFFD5]=0x31;
		ROM[0xFFD6]=0x02;
		S9xMessage(S9X_ERROR,S9X_ROM_CONFUSING_FORMAT_INFO, "Warning! Hacked Dump!");
	}

	if((ROM[0x7FD5]==0x42)&&(ROM[0x7FD6]==0x13)&&(strncmp("METAL COMBAT",(char*)&ROM[0x7FC0],12)==0))
		S9xMessage(S9X_ERROR,S9X_ROM_CONFUSING_FORMAT_INFO, "Warning! Hacked Dump!");
	

    int hi_score, lo_score;
	
    hi_score = ScoreHiROM (FALSE);
    lo_score = ScoreLoROM (FALSE);
    
    if (HeaderCount == 0 && !Settings.ForceNoHeader &&
		((hi_score > lo_score && ScoreHiROM (TRUE) > hi_score) ||
		(hi_score <= lo_score && ScoreLoROM (TRUE) > lo_score)))
    {
		memmove (Memory.ROM, Memory.ROM + 512, TotalFileSize - 512);
		TotalFileSize -= 512;
		S9xMessage (S9X_INFO, S9X_HEADER_WARNING, 
			"Try specifying the -nhd command line option if the game doesn't work\n");
		//modifying ROM, so we need to rescore
		hi_score = ScoreHiROM (FALSE);
		lo_score = ScoreLoROM (FALSE);
    }
	
    CalculatedSize = (TotalFileSize / 0x2000) * 0x2000;
    ZeroMemory (ROM + CalculatedSize, MAX_ROM_SIZE - CalculatedSize);
	
	if (CalculatedSize > 0x400000 &&
			(ROM[0x7fd5] + (ROM[0x7fd6] << 8)) != 0x3423 && // exclude SA-1
			(ROM[0x7fd5] + (ROM[0x7fd6] << 8)) != 0x3523 &&
			(ROM[0x7fd5] + (ROM[0x7fd6] << 8)) != 0x4332 && // exclude S-DD1
			(ROM[0x7fd5] + (ROM[0x7fd6] << 8)) != 0x4532 &&
			(ROM[0xffd5] + (ROM[0xffd6] << 8)) != 0xF93a && // exclude SPC7110
			(ROM[0xffd5] + (ROM[0xffd6] << 8)) != 0xF53a)
			ExtendedFormat = YEAH;

	// if both vectors are invalid, it's type 1 interleaved LoROM
	if (ExtendedFormat == NOPE &&
		((ROM[0x7ffc] + (ROM[0x7ffd] << 8)) < 0x8000) &&
		((ROM[0xfffc] + (ROM[0xfffd] << 8)) < 0x8000))
	{
		if (!Settings.ForceInterleaved && !Settings.ForceNotInterleaved)
			S9xDeinterleaveType1(TotalFileSize, ROM);
	}

	//CalculatedSize is now set, so rescore
	hi_score = ScoreHiROM (FALSE);
	lo_score = ScoreLoROM (FALSE);

	if(ExtendedFormat!=NOPE)
	{
		int swappedlorom, swappedhirom;

		swappedlorom=ScoreLoROM(FALSE, 0x400000);
		swappedhirom=ScoreHiROM(FALSE, 0x400000);

		//set swapped here.

		if(max(swappedlorom, swappedhirom) >= max(lo_score, hi_score))
		{
			ExtendedFormat = BIGFIRST;
			hi_score=swappedhirom;
			lo_score=swappedlorom;
			RomHeader=ROM+0x400000;
		}
		else
			ExtendedFormat = SMALLFIRST;
	}

    Interleaved = Settings.ForceInterleaved || Settings.ForceInterleaved2;
    if (Settings.ForceLoROM || (!Settings.ForceHiROM && lo_score >= hi_score))
    {
		LoROM = TRUE;
		HiROM = FALSE;
		
		// Ignore map type byte if not 0x2x or 0x3x
		if ((RomHeader [0x7fd5] & 0xf0) == 0x20 || (RomHeader [0x7fd5] & 0xf0) == 0x30)
		{
			switch (RomHeader [0x7fd5] & 0xf)
			{
			case 1:
				Interleaved = TRUE;
				break;
			case 5:
				Interleaved = TRUE;
				Tales = TRUE;
				break;
			}
		}
    }
    else
    {
		if ((RomHeader [0xffd5] & 0xf0) == 0x20 || (RomHeader [0xffd5] & 0xf0) == 0x30)
		{
			switch (RomHeader [0xffd5] & 0xf)
			{
			case 0:
			case 3:
				Interleaved = TRUE;
				break;
			}
		}
		LoROM = FALSE;
		HiROM = TRUE;
    }

	// this two games fail to be detected
	if (!Settings.ForceHiROM && !Settings.ForceLoROM)
	{
		if (strncmp((char *) &ROM[0x7fc0], "YUYU NO QUIZ DE GO!GO!", 22) == 0 ||
		   (strncmp((char *) &ROM[0xffc0], "BATMAN--REVENGE JOKER",  21) == 0))
		{
			LoROM = TRUE;
			HiROM = FALSE;
			Interleaved = FALSE;
			Tales = FALSE;
		}
	}

    // More 
    if (!Settings.ForceHiROM && !Settings.ForceLoROM &&
		!Settings.ForceInterleaved && !Settings.ForceInterleaved2 &&
		!Settings.ForceNotInterleaved && !Settings.ForcePAL &&
		!Settings.ForceSuperFX && !Settings.ForceDSP1 &&
		!Settings.ForceSA1 && !Settings.ForceC4 &&
		!Settings.ForceSDD1)
    {


#ifdef DETECT_NASTY_FX_INTERLEAVE
//MK: Damn. YI trips a BRK currently. Maybe even on a real cart.

#ifdef LSB_FIRST 
		if(strncmp((char *) &ROM [0x7fc0], "YOSHI'S ISLAND", 14) == 0&&(*(uint16*)&ROM[0x7FDE])==57611&&ROM[0x10002]==0xA9)
#else
			if(strncmp((char *) &ROM [0x7fc0], "YOSHI'S ISLAND", 14) == 0&&(ROM[0x7FDE]+(ROM[0x7FDF]<<8))==57611&&ROM[0x10002]==0xA9)
#endif
		{
			Interleaved=true;
			Settings.ForceInterleaved2=true;
		}
#endif
    }
	
    if (!Settings.ForceNotInterleaved && Interleaved)
    {
		CPU.TriedInterleavedMode2 = TRUE;
		//S9xMessage (S9X_INFO, S9X_ROM_INTERLEAVED_INFO,
		//	"ROM image is in interleaved format - converting...");
		
		if (Tales)
		{
			if(Memory.ExtendedFormat==BIGFIRST)
			{
				S9xDeinterleaveType1(0x400000, ROM);
				S9xDeinterleaveType1(CalculatedSize-0x400000, ROM+0x400000);
			}
			else
			{
				S9xDeinterleaveType1(CalculatedSize-0x400000, ROM);
				S9xDeinterleaveType1(0x400000, ROM+CalculatedSize-0x400000);
				
			}
			
			LoROM = FALSE;
			HiROM = TRUE;
			
			
		}
		else if (Settings.ForceInterleaved2)
		{
			S9xDeinterleaveType2(FALSE);
		}
		else if (Settings.ForceInterleaveGD24 && CalculatedSize ==0x300000)
		{
			bool8 t = LoROM;
			
			LoROM = HiROM;
			HiROM = t;
			S9xDeinterleaveGD24(CalculatedSize, ROM);
		}
		else
		{

			bool8 t = LoROM;
			
			LoROM = HiROM;
			HiROM = t;
			
			S9xDeinterleaveType1(CalculatedSize, ROM);
		}

		hi_score = ScoreHiROM (FALSE);
		lo_score = ScoreLoROM (FALSE);
		
		if ((HiROM &&
			(lo_score >= hi_score || hi_score < 0)) ||
			(LoROM && 
			(hi_score > lo_score || lo_score < 0)))
		{
			if (retry_count == 0)
			{
				S9xMessage (S9X_INFO, S9X_ROM_CONFUSING_FORMAT_INFO,
					"ROM lied about its type! Trying again.");
				Settings.ForceNotInterleaved = TRUE;
				Settings.ForceInterleaved = FALSE;
				retry_count++;
				goto again;
			}
		}
    }

	if(ExtendedFormat==SMALLFIRST)
		Tales=true;

    //FreeSDD1Data ();
    InitROM (Tales);

	// Updated to load cheats from a text file.
	// The text file takes priority over the original
	// binary format file.
	//
	if (!S9xLoadCheatTextFile (S9xGetFilename("/rom.chx")))
    	S9xLoadCheatFile (S9xGetFilename("/rom.cht"));

    S9xInitCheatData ();
	S9xApplyCheats ();
	
    S9xReset ();
}

uint32 CMemory::FileLoader (uint8* buffer, const char* filename, int32 maxsize)
{
	STREAM ROMFile;
	int32 TotalFileSize = 0;
	int len = 0;
	int nFormat=FILE_DEFAULT;
 
	char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char name [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    char fname [_MAX_PATH + 1];

	unsigned long FileSize = 0;
	
#ifdef UNZIP_SUPPORT
	unzFile file=NULL;
#endif
    
	_splitpath (filename, drive, dir, name, ext);
    _makepath (fname, drive, dir, name, ext);
	
#if defined(__WIN32__) || defined(__MACOSX__)
    memmove (&ext [0], &ext[1], 4);
#endif

	if (strcasecmp (ext, "zip") == 0)
		nFormat = FILE_ZIP;
	else if (strcasecmp (ext, "rar") == 0)
		nFormat = FILE_RAR;
	else
		nFormat = FILE_DEFAULT;


	switch( nFormat )
	{
	case FILE_ZIP:	
	case FILE_RAR:
		// non existant rar loading
		S9xMessage (S9X_ERROR, S9X_ROM_INFO, "Zip/Rar Archives are not currently supported.");
		return (0);
		break;

	case FILE_DEFAULT:
	default:
		// any other roms go here

		if ((ROMFile = OPEN_STREAM (fname, "rb")) == NULL)
			return (0);
		
		strcpy (ROMFilename, fname);
		
		HeaderCount = 0;
		uint8 *ptr = buffer;
		bool8 more = FALSE;
		do
		{
			FileSize = READ_STREAM (ptr, maxsize + 0x200 - (ptr - ROM), ROMFile);
			CLOSE_STREAM (ROMFile);
			
			int calc_size = (FileSize / 0x2000) * 0x2000;
		
			if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
				Settings.ForceHeader)
			{
				memmove (ptr, ptr + 512, calc_size);
				HeaderCount++;
				FileSize -= 512;
			}
			
			ptr += FileSize;
			TotalFileSize += FileSize;
		

			// check for multi file roms

			if (ptr - ROM < maxsize + 0x200 &&
				(isdigit (ext [0]) && ext [1] == 0 && ext [0] < '9'))
			{
				more = TRUE;
				ext [0]++;
#if defined(__WIN32__) || defined(__MACOSX__)
		        memmove (&ext [1], &ext [0], 4);
			    ext [0] = '.';
#endif
				_makepath (fname, drive, dir, name, ext);
			}
			else if (ptr - ROM < maxsize + 0x200 &&
					(((len = strlen (name)) == 7 || len == 8) &&
					strncasecmp (name, "sf", 2) == 0 &&
					isdigit (name [2]) && isdigit (name [3]) && isdigit (name [4]) &&
					isdigit (name [5]) && isalpha (name [len - 1])))
			{
				more = TRUE;
				name [len - 1]++;
#if defined(__WIN32__) || defined(__MACOSX__)
				memmove (&ext [1], &ext [0], 4);
				ext [0] = '.';
#endif
				_makepath (fname, drive, dir, name, ext);
			}
			else
				more = FALSE;

		} while (more && (ROMFile = OPEN_STREAM (fname, "rb")) != NULL);
    
		break;
	}
	return TotalFileSize;
}

#if 0
/**********************************************************************************************/
/* LoadMulti()                                                                                */
/* This function loads a Slotted SNES-Backup image and fills the slot.                        */
/**********************************************************************************************/

bool8 CMemory::LoadMulti (const char *basename, const char *slot1name, const char *slot2name)
{
    unsigned long FileSize = 0;
	
	if(*basename=='\0')
		return FALSE;
	
	SufamiTurbo=TRUE;
	
	int32 offset;

    memset (&SNESGameFixes, 0, sizeof(SNESGameFixes));
    SNESGameFixes.SRAMInitialValue = 0x60;
	
    memset (bytes0x2000, 0, 0x2000);
	
    CalculatedSize = 0;
	
	
    int32 TotalFileSize = FileLoader(ROM, basename, MAX_ROM_SIZE);
	
	if(0== TotalFileSize)
		return FALSE;
	else CheckForIPSPatch (basename, HeaderCount != 0, TotalFileSize);
	
	CalculatedSize=TotalFileSize;

	for(offset=0; offset<TotalFileSize; offset+=0x100000);

	//insert base type test here.
	
	if(slot1name[0]!='\0')
	{
		
		TotalFileSize = FileLoader(ROM+offset, slot1name, MAX_ROM_SIZE);
		
		if(0== TotalFileSize)
			return FALSE;
		else CheckForIPSPatch (slot1name, HeaderCount != 0, TotalFileSize);
		ROMOffset1=&ROM[offset];
		Slot1Size=TotalFileSize;
	}
	int32 temp=offset;
	for(; offset<temp+TotalFileSize; offset+=0x100000);

	if(slot2name[0]!='\0')
	{
		TotalFileSize = FileLoader(ROM+offset, slot2name, MAX_ROM_SIZE);
		
		if(0== TotalFileSize)
			return FALSE;
		else CheckForIPSPatch (slot2name, HeaderCount != 0, TotalFileSize);
		ROMOffset2=&ROM[offset];
		Slot2Size=TotalFileSize;
	}

    InitROM (FALSE);
    S9xLoadCheatFile (S9xGetFilename(".cht"));
    S9xInitCheatData ();
    S9xApplyCheats ();
	
    S9xReset ();
	
    return (TRUE);
}

bool8 SufamiTurboBIOSSig(uint8* file, int32 size)
{
	if(!strcmp((char*)file, "BANDAI SFC-ADX")&&!strcmp((char*)(file+0x10), "SFC-ADX BACKUP"))
	{
		//possible match.
		//check size
		if(size!=0x40000)
			return FALSE;
		//and CRC32
		if(0x9B4CA911==caCRC32(file, size))
		{
			return TRUE;
		}

	}
	return FALSE;
}

bool8 SufamiTurboCartSig(uint8* file, int32 size)
{
	//test not a BIOS
	if(!strcmp((char*)file, "BANDAI SFC-ADX")&&strcmp((char*)(file+0x10), "SFC-ADX BACKUP"))
	{
		//possible match.
		//check size
		if(size>0x100000||size <0x80000)
			return FALSE;
		//probably a minicart
		return TRUE;
	}
	return FALSE;
}

bool8 SameGameSig(uint8* file, int32 size)
{
	//preheader sig
	if(strcmp((char*)(file+0xFFA0),"1995/12/16 10:2018ZS5J"))
		return FALSE;
	if(size!=0x100000)
		return FALSE;
	if(0x133E1C5B==caCRC32(file, size))
		return TRUE;
	return FALSE;
}
bool8 GNextSig(uint8* file, int32 size)
{
	//preheader sig
	if(strcmp((char*)(file+0xFFAA),"GNEXT B2ZX3J"))
		return FALSE;
	if(size!=0x180000)
		return FALSE;
	if(0x845E420D==caCRC32(file, size))
		return TRUE;
	return FALSE;
}
int MultiType(uint8* file, int32 size)
{
	//check for ST signiture
	if(SufamiTurboBIOSSig(file, size))
		return 1;
	//check for Same Game signiture
	if(SameGameSig(file, size))
		return 2;
	//check for G-Next signiture
	if(GNextSig(file, size))
		return 3;
	return 0;
}

#endif

//compatibility wrapper
void S9xDeinterleaveMode2 ()
{
	S9xDeinterleaveType2();
}

void S9xDeinterleaveType2 (bool8 reset)
{

    S9xMessage (S9X_INFO, S9X_ROM_INTERLEAVED_INFO,
		"ROM image is in interleaved format - converting...");
	
    int nblocks = Memory.CalculatedSize >> 16;
    int step = 64;
	
    while (nblocks <= step)
		step >>= 1;
	
    nblocks = step;
    uint8 blocks [256];
    int i;
	
    for (i = 0; i < nblocks * 2; i++)
    {
		blocks [i] = (i & ~0xF) | ((i & 3) << 2) |
			((i & 12) >> 2);
    }
	
    uint8 *tmp = (uint8 *) malloc (0x10000);
	
    if (tmp)
    {
		for (i = 0; i < nblocks * 2; i++)
		{
			for (int j = i; j < nblocks * 2; j++)
			{
				if (blocks [j] == i)
				{
					memmove (tmp, &Memory.ROM [blocks [j] * 0x10000], 0x10000);
					memmove (&Memory.ROM [blocks [j] * 0x10000], 
						&Memory.ROM [blocks [i] * 0x10000], 0x10000);
					memmove (&Memory.ROM [blocks [i] * 0x10000], tmp, 0x10000);
					uint8 b = blocks [j];
					blocks [j] = blocks [i];
					blocks [i] = b;
					break;
				}
			}
		}
		free ((char *) tmp);
		tmp=NULL;
    }
	if(reset)
	{
	    Memory.InitROM (FALSE);
		S9xReset ();
	}
}

//CRC32 for char arrays
inline uint32 caCRC32(uint8 *array, uint32 size, register uint32 crc32)
{
  for (register uint32 i = 0; i < size; i++)
  {
    crc32 = ((crc32 >> 8) & 0x00FFFFFF) ^ crc32Table[(crc32 ^ array[i]) & 0xFF];
  }
  return ~crc32;
}

void CMemory::InitROM (bool8 Interleaved)
{
    SuperFX.nRomBanks = CalculatedSize >> 15;
    Settings.MultiPlayer5Master = Settings.MultiPlayer5;
    Settings.MouseMaster = Settings.Mouse;
    Settings.SuperScopeMaster = Settings.SuperScope;
    Settings.DSP1Master = Settings.ForceDSP1;
    Settings.SuperFX = FALSE;
    Settings.SA1 = FALSE;
    Settings.C4 = FALSE;
    Settings.SDD1 = FALSE;
    Settings.SRTC = FALSE;
	Settings.SPC7110=FALSE;
	Settings.SPC7110RTC=FALSE;
	Settings.BS=FALSE;
	Settings.OBC1=FALSE;
	Settings.SETA=FALSE;
	Settings.DSP=0;
	s7r.DataRomSize = 0;
	CalculatedChecksum=0;
	uint8* RomHeader;

	RomHeader=ROM+0x7FB0;

	if(ExtendedFormat==BIGFIRST)
		RomHeader+=0x400000;

	if(HiROM)
		RomHeader+=0x8000;


    ZeroMemory (BlockIsRAM, MEMMAP_NUM_BLOCKS);
    ZeroMemory (BlockIsROM, MEMMAP_NUM_BLOCKS);

    ::SRAM = SRAM;
    memset (ROMId, 0, 5);
    memset (CompanyId, 0, 3);

	S9xInitBSX(); // Set BS header before parsing

	ParseSNESHeader(RomHeader);

	   // DSP1/2/3/4
   if (Memory.ROMType == 0x03)
   {
      if (Memory.ROMSpeed == 0x30)
         Settings.DSP = 4; // DSP4
      else
         Settings.DSP = 1; // DSP1
   }
   else if (Memory.ROMType == 0x05)
   {
      if (Memory.ROMSpeed == 0x20)
         Settings.DSP = 2; // DSP2
      else if (Memory.ROMSpeed == 0x30 && RomHeader[0x2a] == 0xb2)
         Settings.DSP = 3; // DSP3
      else
         Settings.DSP = 1; // DSP1
   }

   switch (Settings.DSP)
	{
		case 1:	// DSP1
			if (HiROM)
			{
				DSP0.boundary = 0x7000;
				DSP0.maptype = M_DSP1_HIROM;
			}
			else
			if (CalculatedSize > 0x100000)
			{
				DSP0.boundary = 0x4000;
				DSP0.maptype = M_DSP1_LOROM_L;
			}
			else
			{
				DSP0.boundary = 0xc000;
				DSP0.maptype = M_DSP1_LOROM_S;
			}

			SetDSP = &DSP1SetByte;
			GetDSP = &DSP1GetByte;
			break;

		case 2: // DSP2
			DSP0.boundary = 0x10000;
			DSP0.maptype = M_DSP2_LOROM;
			SetDSP = &DSP2SetByte;
			GetDSP = &DSP2GetByte;
			break;

		case 3: // DSP3
			DSP0.boundary = 0xc000;
			DSP0.maptype = M_DSP3_LOROM;
			SetDSP = &DSP3SetByte;
			GetDSP = &DSP3GetByte;
			break;

		case 4: // DSP4
			DSP0.boundary = 0xc000;
			DSP0.maptype = M_DSP4_LOROM;
			SetDSP = &DSP4SetByte;
			GetDSP = &DSP4GetByte;
			break;

		default:
			SetDSP = NULL;
			GetDSP = NULL;
			break;
	}

	// Try to auto-detect the DSP1 chip
	if (!Settings.ForceNoDSP1 &&
			(ROMType & 0xf) >= 3 && (ROMType & 0xf0) == 0)
			Settings.DSP1Master = TRUE;
	
	if (Memory.HiROM)
    {
	    // Enable S-RTC (Real Time Clock) emulation for Dai Kaijyu Monogatari 2
	    Settings.SRTC = ((ROMType & 0xf0) >> 4) == 5;

		if(((ROMSpeed&0x0F)==0x0A)&&((ROMType&0xF0)==0xF0))
		{
			Settings.SPC7110=true;
			if((ROMType&0x0F)==0x09)
				Settings.SPC7110RTC=true;
		}
		
		if (Settings.BS)
			;//BSHiROMMap ();
		else if(Settings.SPC7110)
		{
			SPC7110HiROMMap();
		}
		else if ((ROMSpeed & ~0x10) == 0x25)
		{
			TalesROMMap (Interleaved);
		}
		else 
			HiROMMap ();
    }
    else
    {
		Settings.SuperFX = Settings.ForceSuperFX;
		
		if(ROMType==0x25)
			Settings.OBC1=TRUE;

		//BS-X BIOS
		if(ROMType==0xE5)
			Settings.BS=TRUE;

		if ((ROMType & 0xf0) == 0x10)
			Settings.SuperFX = !Settings.ForceNoSuperFX;
		//OBC1 hack ROM
		if (strncmp(Memory.ROMName, "METAL COMBAT", 12) == 0 &&
			Memory.ROMType == 0x13 && Memory.ROMSpeed == 0x42)
		{
			Settings.OBC1 = true;
			Settings.SuperFX = Settings.ForceSuperFX;
			Memory.ROMSpeed = 0x30;
		}
		
		Settings.SDD1 = Settings.ForceSDD1;
		if ((ROMType & 0xf0) == 0x40)
			Settings.SDD1 = !Settings.ForceNoSDD1;

		if(((ROMType &0xF0) == 0xF0)&((ROMSpeed&0x0F)!=5))
		{
			SRAMSize=2;
			SNESGameFixes.SRAMInitialValue = 0x00;
			if((ROMType &0x0F)==6)
			{
				if(ROM[0x7FD7]==0x09)
				{
					Settings.SETA=ST_011;
					SetSETA=&S9xSetST011;
					GetSETA=&S9xGetST011;
				}
				else
				{
					Settings.SETA=ST_010;
					SetSETA=&S9xSetST010;
					GetSETA=&S9xGetST010;
				}
			}
			else
			{
				Settings.SETA=ST_018;
				SRAMSize=2;
			}
		}
		Settings.C4 = Settings.ForceC4;
		if ((ROMType & 0xf0) == 0xf0 &&
            (strncmp (ROMName, "MEGAMAN X", 9) == 0 ||
			strncmp (ROMName, "ROCKMAN X", 9) == 0))
		{
			Settings.C4 = !Settings.ForceNoC4;
		}
		
		if(Settings.SETA&&Settings.SETA!=ST_018)
		{
			SetaDSPMap();
		}
		else if (Settings.SuperFX)
		{
			//::SRAM = ROM + 1024 * 1024 * 4;
			SuperFXROMMap ();
			Settings.MultiPlayer5Master = FALSE;
			//Settings.MouseMaster = FALSE;
			//Settings.SuperScopeMaster = FALSE;
			Settings.DSP1Master = FALSE;
			Settings.SA1 = FALSE;
			Settings.C4 = FALSE;
			Settings.SDD1 = FALSE;
		}
		else if (Settings.ForceSA1 ||
			(!Settings.ForceNoSA1 && (ROMSpeed & ~0x10) == 0x23 && 
			(ROMType & 0xf) > 3 && (ROMType & 0xf0) == 0x30))
		{
			Settings.SA1 = TRUE;
//			Settings.MultiPlayer5Master = FALSE;
			//Settings.MouseMaster = FALSE;
			//Settings.SuperScopeMaster = FALSE;
			Settings.DSP1Master = FALSE;
			Settings.C4 = FALSE;
			Settings.SDD1 = FALSE;
			SA1ROMMap ();
		}
		else if ((ROMSpeed & ~0x10) == 0x25)
			TalesROMMap (Interleaved);
		else if(ExtendedFormat!=NOPE)
			JumboLoROMMap(Interleaved);
		else if (strncmp ((char *) &Memory.ROM [0x7fc0], "SOUND NOVEL-TCOOL", 17) == 0 ||
			strncmp ((char *) &Memory.ROM [0x7fc0], "DERBY STALLION 96", 17) == 0)
		{
			LoROM24MBSMap ();
			Settings.DSP1Master = FALSE;
		}

		else if (strncmp ((char *) &Memory.ROM [0x7fc0], "THOROUGHBRED BREEDER3", 21) == 0 ||
			strncmp ((char *) &Memory.ROM [0x7fc0], "RPG-TCOOL 2", 11) == 0)
		{
			SRAM512KLoROMMap ();
			Settings.DSP1Master = FALSE;
		}
		else if (strncmp ((char *) &Memory.ROM [0x7fc0], "ADD-ON BASE CASSETE", 19) == 0)
		{
			Settings.MultiPlayer5Master = FALSE;
			Settings.MouseMaster = FALSE;
			Settings.SuperScopeMaster = FALSE;
			Settings.DSP1Master = FALSE;
			SufamiTurboLoROMMap(); 
			Memory.SRAMSize = 3;
		}
		else if ((ROMSpeed & ~0x10) == 0x22 &&
			strncmp (ROMName, "Super Street Fighter", 20) != 0)
		{
			AlphaROMMap ();
		}
		else if (Settings.BS)
			;//BSLoROMMap();
		else LoROMMap ();
    }

	if(Settings.BS)
		ROMRegion=0;

	uint32 sum1 = 0;
	uint32 sum2 = 0;
	if(0==CalculatedChecksum)
	{
		int power2 = 0;
		int size = CalculatedSize;
		
		while (size >>= 1)
			power2++;
	
		size = 1 << power2;
		uint32 remainder = CalculatedSize - size;
	
	
		int i;
	
		for (i = 0; i < size; i++)
			sum1 += ROM [i];
	
		for (i = 0; i < (int) remainder; i++)
			sum2 += ROM [size + i];
	
		int sub = 0;
		if (Settings.BS&& ROMType!=0xE5)
		{
			if (Memory.HiROM)
			{
				for (i = 0; i < 48; i++)
					sub += ROM[0xffb0 + i];
			}
			else if (Memory.LoROM)
			{
				for (i = 0; i < 48; i++)
					sub += ROM[0x7fb0 + i];
			}
			sum1 -= sub;
		}


	    if (remainder)
    		{
				sum1 += sum2 * (size / remainder);
    		}
	
	
    sum1 &= 0xffff;
    Memory.CalculatedChecksum=sum1;
	}
    //now take a CRC32
    ROMCRC32 = caCRC32(ROM, CalculatedSize);

	// NTSC/PAL
	if (Settings.ForceNTSC)
		Settings.PAL = FALSE;
	else
	if (Settings.ForcePAL)
		Settings.PAL = TRUE;
	else
	if (!Settings.BS && (ROMRegion >= 2) && (ROMRegion <= 12))
		Settings.PAL = TRUE;
	else
		Settings.PAL = FALSE;

	if (Settings.PAL)
	{
		Settings.FrameTime = Settings.FrameTimePAL;
		Memory.ROMFramesPerSecond = 50;
	}
	else
	{
		Settings.FrameTime = Settings.FrameTimeNTSC;
		Memory.ROMFramesPerSecond = 60;
	}
	
	ROMName[ROM_NAME_LEN - 1] = 0;
	if (strlen (ROMName))
	{
		char *p = ROMName + strlen (ROMName) - 1;
		
		while (p > ROMName && *(p - 1) == ' ')
			p--;
		*p = 0;
	}
	
	{
		SRAMMask = Memory.SRAMSize ?
			((1 << (Memory.SRAMSize + 3)) * 128) - 1 : 0;
	}

	IAPU.OneCycle = ONE_APU_CYCLE;
	Settings.Shutdown = Settings.ShutdownMaster;
	

	ResetSpeedMap();
	ApplyROMFixes ();
	sprintf (ROMName, "%s", Safe (ROMName));
	sprintf (ROMId, "%s", Safe (ROMId));
	sprintf (CompanyId, "%s", Safe (CompanyId));
	
		sprintf (String, "\"%s\" [%s] %s, %s, Type: %s, Mode: %s, TV: %s, S-RAM: %s, ROMId: %s Company: %2.2s CRC32: %08X",
		ROMName,
		(ROMChecksum + ROMComplementChecksum != 0xffff ||
		ROMChecksum != CalculatedChecksum) ? "bad checksum" : "checksum ok",
		MapType (),
		Size (),
		KartContents (),
		MapMode (),
		TVStandard (),
		StaticRAMSize (),
		ROMId,
		CompanyId,
		ROMCRC32);
	
	S9xMessage (S9X_INFO, S9X_ROM_INFO, String);

	Settings.ForceHeader = Settings.ForceHiROM = Settings.ForceLoROM = 
		Settings.ForceInterleaved = Settings.ForceNoHeader = Settings.ForceNotInterleaved = 
		Settings.ForceInterleaved2=false;
}

bool8 CMemory::LoadSRAM (const char *filename)
{
    int size = Memory.SRAMSize ?
	       (1 << (Memory.SRAMSize + 3)) * 128 : 0;
	
    memset (SRAM, SNESGameFixes.SRAMInitialValue, 0x20000);
	
    if (size > 0x20000)
		size = 0x20000;
	
    if (size)
    {
		FILE *file;
		if ((file = fopen (filename, "rb")))
		{
			int len = fread ((char*) ::SRAM, 1, 0x20000, file);
			fclose (file);
			if (len - size == 512)
			{
				// S-RAM file has a header - remove it
				memmove (::SRAM, ::SRAM + 512, size);
			}
			if (len == size + SRTC_SRAM_PAD)
			{
				S9xSRTCPostLoadState ();
				S9xResetSRTC ();
				rtc.index = -1;
				rtc.mode = MODE_READ;
			}
			else
				S9xHardResetSRTC ();
			
			if(Settings.SPC7110RTC)
			{
				S9xLoadSPC7110RTC (&rtc_f9);
			}
			
			return (TRUE);
		}
		S9xHardResetSRTC ();
		return (FALSE);
    }
//    if (Settings.SDD1)
//		S9xSDD1LoadLoggedData ();
	
    return (TRUE);
}

bool8 CMemory::SaveSRAM (const char *filename)
{
	if(Settings.SuperFX && Memory.ROMType < 0x15)
		return TRUE;
	if(Settings.SA1 && Memory.ROMType == 0x34)
		return TRUE;

    int size = Memory.SRAMSize ?
	       (1 << (Memory.SRAMSize + 3)) * 128 : 0;
    if (Settings.SRTC)
    {
		size += SRTC_SRAM_PAD;
		S9xSRTCPreSaveState ();
    }
	
 //   if (Settings.SDD1)
//		S9xSDD1SaveLoggedData ();
	
    if (size > 0x20000)
		size = 0x20000;
	
    if (size && *Memory.ROMFilename)
    {
		FILE *file;
		if ((file = fopen (filename, "wb")))
		{
			fwrite ((char *) ::SRAM, size, 1, file);
			fclose (file);
#if defined(__linux)
			chown (filename, getuid (), getgid ());
#endif
			if(Settings.SPC7110RTC)
			{
				S9xSaveSPC7110RTC (&rtc_f9);
			}
			return (TRUE);
		}
    }
    return (FALSE);
}

void CMemory::FixROMSpeed ()
{
    int c;

	if(CPU.FastROMSpeed==0)
		CPU.FastROMSpeed=SLOW_ONE_CYCLE;
	

    for (c = 0x800; c < 0x1000; c++)
    {
		if (c&0x8 || c&0x400)
			MemorySpeed [c] = (uint8) CPU.FastROMSpeed;
    }
}


void CMemory::ResetSpeedMap()
{
	int i;
	memset(MemorySpeed, SLOW_ONE_CYCLE, 0x1000);
	for(i=0;i<0x400;i+=0x10)
	{
		MemorySpeed[i+2]=MemorySpeed[0x800+i+2]= ONE_CYCLE;
		MemorySpeed[i+3]=MemorySpeed[0x800+i+3]= ONE_CYCLE;
		MemorySpeed[i+4]=MemorySpeed[0x800+i+4]= ONE_CYCLE;
		MemorySpeed[i+5]=MemorySpeed[0x800+i+5]= ONE_CYCLE;
	}
	CMemory::FixROMSpeed ();
}

void CMemory::WriteProtectROM ()
{
    memmove ((void *) WriteMap, (void *) Map, sizeof (Map));
    for (int c = 0; c < 0x1000; c++)
    {
		if (BlockIsROM [c])
			WriteMap [c] = (uint8 *) MAP_NONE;
    }
}

void CMemory::MapRAM ()
{
    int c;

	if(Memory.LoROM&&!Settings.SDD1)
	{
		// Banks 70->77, S-RAM
		for (c = 0; c < 0x0f; c++)
		{
			for(int i=0;i<8;i++)
			{
				Map [(c<<4) + 0xF00+i]=Map [(c<<4) + 0x700+i] = (uint8 *) MAP_LOROM_SRAM;
				BlockIsRAM [(c<<4) + 0xF00+i] =BlockIsRAM [(c<<4) + 0x700+i] = TRUE;
				BlockIsROM [(c<<4) + 0xF00+i] =BlockIsROM [(c<<4) + 0x700+i] = FALSE;
			}
		}
	}
	else if(Memory.LoROM&&Settings.SDD1)
	{
		// Banks 70->77, S-RAM
		for (c = 0; c < 0x0f; c++)
		{
			for(int i=0;i<8;i++)
			{
				Map [(c<<4) + 0x700+i] = (uint8 *) MAP_LOROM_SRAM;
				BlockIsRAM [(c<<4) + 0x700+i] = TRUE;
				BlockIsROM [(c<<4) + 0x700+i] = FALSE;
			}
		}
	}
    // Banks 7e->7f, RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
	WriteProtectROM ();
}

void CMemory::MapExtraRAM ()
{
    int c;
	
    // Banks 7e->7f, RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
	
    // Banks 70->73, S-RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x700] = ::SRAM;
		Map [c + 0x710] = ::SRAM + 0x8000;
		Map [c + 0x720] = ::SRAM + 0x10000;
		Map [c + 0x730] = ::SRAM + 0x18000;
		
		BlockIsRAM [c + 0x700] = TRUE;
		BlockIsROM [c + 0x700] = FALSE;
		BlockIsRAM [c + 0x710] = TRUE;
		BlockIsROM [c + 0x710] = FALSE;
		BlockIsRAM [c + 0x720] = TRUE;
		BlockIsROM [c + 0x720] = FALSE;
		BlockIsRAM [c + 0x730] = TRUE;
		BlockIsROM [c + 0x730] = FALSE;
    }
}

void CMemory::LoROMMap ()
{
    int c;
    int i;

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		if(Settings.SETA==ST_018)
			Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_SETA_RISC;
		else Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		if (Settings.DSP1Master)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_DSP;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_DSP;
		}
		else if (Settings.C4)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_C4;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_C4;
		}
		else if(Settings.OBC1)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_OBC_RAM;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_OBC_RAM;
		}
		else
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) bytes0x2000 - 0x6000;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) bytes0x2000 - 0x6000;
		}
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 11) % CalculatedSize] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    if (Settings.DSP1Master)
    {
		// Banks 30->3f and b0->bf
		for (c = 0x300; c < 0x400; c += 16)
		{
			for (i = c + 8; i < c + 16; i++)
			{
				Map [i] = Map [i + 0x800] = (uint8 *) MAP_DSP;
				BlockIsROM [i] = BlockIsROM [i + 0x800] = FALSE;
			}
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) % CalculatedSize];
		
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [((c << 11) + 0x200000) % CalculatedSize] - 0x8000;
		
		for (i = c; i < c + 16; i++)	
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
    }
	
    if (Settings.DSP1Master)
    {
		for (c = 0; c < 0x100; c++)
		{
			Map [c + 0xe00] = (uint8 *) MAP_DSP;
			BlockIsROM [c + 0xe00] = FALSE;
		}
    }

    MapRAM ();
    WriteProtectROM ();
}

void CMemory::SetaDSPMap ()
{
    int c;
    int i;
 
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) bytes0x2000 - 0x6000;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) bytes0x2000 - 0x6000;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 11) % CalculatedSize] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [((c << 11) + 0x200000) % CalculatedSize] - 0x8000;
		
		//only upper half is ROM
		for (i = c+8; i < c + 16; i++)	
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
    }
	
	memset(SRAM, 0, 0x1000);
	for (c=0x600;c<0x680;c+=0x10)
	{
		for(i=0;i<0x08;i++)
		{
			//where does the SETA chip access, anyway?
			//please confirm this?
			Map[c+0x80+i]=(uint8*)MAP_SETA_DSP;
			BlockIsROM [c+0x80+i] = FALSE;
			BlockIsRAM [c+0x80+i] = TRUE;
		}
		
		for(i=0;i<0x04;i++)
		{
			//and this!
			Map[c+i]=(uint8*)MAP_SETA_DSP;
			BlockIsROM [c+i] = FALSE;
		}
	}

    MapRAM ();
    WriteProtectROM ();
}
/*
void CMemory::BSLoROMMap ()
{
    int c;
    int i;
	
	if(Settings.BS)
		SRAMSize=5;

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
BlockIsRAM [c + 5] = BlockIsRAM [c + 0x805] = TRUE;
		
//		Map [c + 6] = Map [c + 0x806] = (uint8 *)MAP_NONE;
//		Map [c + 7] = Map [c + 0x807] = (uint8 *)MAP_NONE;
				Map [c + 6] = Map [c + 0x806] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
BlockIsRAM [c + 6] = BlockIsRAM [c + 0x806] = TRUE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
BlockIsRAM [c + 7] = BlockIsRAM [c + 0x807] = TRUE;
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 11) % CalculatedSize] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }

	for(c=0;c<8;c++)
	{
		Map[(c<<4)+0x105]=(uint8*)MAP_LOROM_SRAM;
		BlockIsROM [(c<<4)+0x105] = FALSE;
		BlockIsRAM [(c<<4)+0x105] = TRUE;
	}
	
  	
  /*  // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) % CalculatedSize];
		
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [((c << 11) + 0x200000) % CalculatedSize - 0x8000];
		
		for (i = c; i < c + 16; i++)	
		{
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	*-/
	for(c=1;c<=4;c++)
	{
		for(i=0;i<16; i++)
		{
			Map[0x400+i+(c<<4)]=(uint8*)MAP_LOROM_SRAM;
			BlockIsRAM[0x400+i+(c<<4)]=TRUE;
			BlockIsROM[0x400+i+(c<<4)]=FALSE;
		}
	}

	for(i=0;i<0x80;i++)
	{
		Map[0x700+i]=&BSRAM[0x10000*(i/16)];
		BlockIsRAM[0x700+i]=TRUE;
		BlockIsROM[0x700+i]=FALSE;
	}
	for (i=0; i<8;i++)
	{
		Map[0x205+(i<<4)]=Map[0x285+(i<<4)]=Map[0x305+(i<<4)]=Map[0x385+(i<<4)]=Map[0x705+(i<<4)];
		BlockIsRAM[0x205+(i<<4)]=BlockIsRAM[0x285+(i<<4)]=BlockIsRAM[0x305+(i<<4)]=BlockIsRAM[0x385+(i<<4)]=TRUE;
		BlockIsROM[0x205+(i<<4)]=BlockIsROM[0x285+(i<<4)]=BlockIsROM[0x305+(i<<4)]=BlockIsROM[0x385+(i<<4)]=FALSE;
	}
	for(c=0;c<8;c++)
	{
		Map[(c<<4)+0x005]=BSRAM-0x5000;
		BlockIsROM [(c<<4)+0x005] = FALSE;
		BlockIsRAM [(c<<4)+0x005] = TRUE;
	}	
    MapRAM ();
    WriteProtectROM ();


}
*/
void CMemory::HiROMMap ()
{
    int i;
	int c;

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		
		if (Settings.DSP1Master)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_DSP;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_DSP;
		}
		else
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		}
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 30->3f and b0->bf, address ranges 6000->7fff is S-RAM.
    for (c = 0; c < 16; c++)
    {
		Map [0x306 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0x307 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0xb06 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0xb07 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		BlockIsRAM [0x306 + (c << 4)] = TRUE;
		BlockIsRAM [0x307 + (c << 4)] = TRUE;
		BlockIsRAM [0xb06 + (c << 4)] = TRUE;
		BlockIsRAM [0xb07 + (c << 4)] = TRUE;
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
		{
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }

    MapRAM ();
    WriteProtectROM ();
}

void CMemory::TalesROMMap (bool8 Interleaved)
{
	  int c;
    int i;
	
    uint32 OFFSET0 = 0x400000;
    uint32 OFFSET1 = 0x400000;
    uint32 OFFSET2 = 0x000000;
	
    if (Interleaved)
    {
		OFFSET0 = 0x000000;
		OFFSET1 = 0x000000;
		OFFSET2 = CalculatedSize-0x400000; //changed to work with interleaved DKJM2.
    }
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;

		//makes more sense to map the range here.
		//ToP seems to use sram to skip intro???
		if(c>=0x300)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_HIROM_SRAM;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_HIROM_SRAM;
			BlockIsRAM [6 + c] = BlockIsRAM [7 + c] =
				BlockIsRAM [0x806 + c]= BlockIsRAM [0x807 + c] = TRUE;
		}
		else
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;

		}
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = &ROM [((c << 12) % (CalculatedSize-0x400000)) + OFFSET0];
			Map [i + 0x800] = &ROM [((c << 12) % 0x400000) + OFFSET2];
			BlockIsROM [i] = TRUE;
			BlockIsROM [i + 0x800] = TRUE;
		}
    }

    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
		{
			Map [i + 0x400] = &ROM [((c << 12) % (CalculatedSize-0x400000)) + OFFSET1];
			Map [i + 0x408] = &ROM [((c << 12) % (CalculatedSize-0x400000)) + OFFSET1];
			Map [i + 0xc00] = &ROM [((c << 12) %0x400000)+ OFFSET2];
			Map [i + 0xc08] = &ROM [((c << 12) % 0x400000) + OFFSET2];
			BlockIsROM [i + 0x400] = TRUE;
			BlockIsROM [i + 0x408] = TRUE;
			BlockIsROM [i + 0xc00] = TRUE;
			BlockIsROM [i + 0xc08] = TRUE;
		}
    }


	ROMChecksum = *(Map[8]+0xFFDE) + (*(Map[8]+0xFFDF) << 8);
	ROMComplementChecksum = *(Map[8]+0xFFDC) + (*(Map[8]+0xFFDD) << 8);

int sum=0;
for(i=0x40;i<0x80; i++)
{
	uint8 * bank_low=(uint8*)Map[i<<4];
	uint8 * bank_high=(uint8*)Map[(i<<4)+0x800];
	for (c=0;c<0x10000; c++)
	{
		sum+=bank_low[c];
		sum+=bank_high[c];
	}
}

CalculatedChecksum=sum&0xFFFF;

    MapRAM ();
    WriteProtectROM ();
}

void CMemory::AlphaROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
	
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
		{
			Map [i + 0x400] = &ROM [(c << 12) % CalculatedSize];
			Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    MapRAM ();
    WriteProtectROM ();
}

void DetectSuperFxRamSize()
{
	if(ROM[0x7FDA]==0x33)
	{
		Memory.SRAMSize=ROM[0x7FBD];
	}
	else
	{
		if(strncmp(Memory.ROMName, "STAR FOX 2", 10)==0)
		{
			Memory.SRAMSize=6;
		}
		else Memory.SRAMSize=5;
	}
}

void CMemory::SuperFXROMMap ()
{
    int c;
    int i;
 
	DetectSuperFxRamSize();

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [0x006 + c] = Map [0x806 + c] = (uint8 *) ::SRAM - 0x6000;
		Map [0x007 + c] = Map [0x807 + c] = (uint8 *) ::SRAM - 0x6000;
		BlockIsRAM [0x006 + c] = BlockIsRAM [0x007 + c] = BlockIsRAM [0x806 + c] = BlockIsRAM [0x807 + c] = TRUE;

		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
    
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
		{
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    // Banks 7e->7f, RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
	
    // Banks 70->71, S-RAM
    for (c = 0; c < 32; c++)
    {
		Map [c + 0x700] = ::SRAM + (((c >> 4) & 1) << 16);
		BlockIsRAM [c + 0x700] = TRUE;
		BlockIsROM [c + 0x700] = FALSE;
    }
	
    // Replicate the first 2Mb of the ROM at ROM + 2MB such that each 32K
    // block is repeated twice in each 64K block.
    for (c = 0; c < 64; c++)
    {
		memmove (&ROM [0x200000 + c * 0x10000], &ROM [c * 0x8000], 0x8000);
		memmove (&ROM [0x208000 + c * 0x10000], &ROM [c * 0x8000], 0x8000);
    }
	
    WriteProtectROM ();
}

void CMemory::SA1ROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) &Memory.FillRAM [0x3000] - 0x3000;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_BWRAM;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_BWRAM;
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 40->7f
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
			Map [i + 0x400] = (uint8 *) &SRAM [(c << 12) & 0x1ffff];
		
		for (i = c; i < c + 16; i++)
		{
			BlockIsROM [i + 0x400] = FALSE;
		}
    }
	
    // c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c;  i < c + 16; i++)
		{
			Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
    WriteProtectROM ();
	
    // Now copy the map and correct it for the SA1 CPU.
    memmove ((void *) SA1.WriteMap, (void *) WriteMap, sizeof (WriteMap));
    memmove ((void *) SA1.Map, (void *) Map, sizeof (Map));
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		SA1.Map [c + 0] = SA1.Map [c + 0x800] = &Memory.FillRAM [0x3000];
		SA1.Map [c + 1] = SA1.Map [c + 0x801] = (uint8 *) MAP_NONE;
		SA1.WriteMap [c + 0] = SA1.WriteMap [c + 0x800] = &Memory.FillRAM [0x3000];
		SA1.WriteMap [c + 1] = SA1.WriteMap [c + 0x801] = (uint8 *) MAP_NONE;
    }
	
    // Banks 60->6f
    for (c = 0; c < 0x100; c++)
		SA1.Map [c + 0x600] = SA1.WriteMap [c + 0x600] = (uint8 *) MAP_BWRAM_BITMAP;
    
    BWRAM = SRAM;
}

void CMemory::LoROM24MBSMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
        Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
        Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x200; c += 16)
    {
		Map [c + 0x800] = RAM;
		Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 0x805] = (uint8 *) MAP_CPU;
        Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 0x807] = (uint8 *) MAP_NONE;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i + 0x800] = &ROM [c << 11] - 0x8000 + 0x200000;
			BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000];
		
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000 - 0x8000];
		
		for (i = c; i < c + 16; i++)
		{
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    MapExtraRAM ();
    WriteProtectROM ();
}

void CMemory::SufamiTurboLoROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000];
		
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000 - 0x8000];
		
		for (i = c; i < c + 16; i++)
		{
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    if (Settings.DSP1Master)
    {
		for (c = 0; c < 0x100; c++)
		{
			Map [c + 0xe00] = (uint8 *) MAP_DSP;
			BlockIsROM [c + 0xe00] = FALSE;
		}
    }
	
    // Banks 7e->7f, RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
	
    // Banks 60->67, S-RAM
    for (c = 0; c < 0x80; c++)
    {
		Map [c + 0x600] = (uint8 *) MAP_LOROM_SRAM;
		BlockIsRAM [c + 0x600] = TRUE;
		BlockIsROM [c + 0x600] = FALSE;
    }
	
    WriteProtectROM ();
}

#if 0

//untested!!
void CMemory::SameGameMap ()
{
    int i;
	int c;
    int j;

	int mask[4];
	int mask2[4];
	for (j=0; j<4; j++)
		mask[j]=mask2[j]=0x00ff;

	mask[0]=(CalculatedSize/0x10000)-1;
	mask2[0]=(Slot1Size/0x10000)-1;

	int x;
	bool foundZeros;
	bool pastZeros;
	
	for(j=0;j<3;j++)
	{
		x=1;
		foundZeros=false;
		pastZeros=false;

		mask[j+1]=mask[j];

		while (x>0x100&&!pastZeros)
		{
			if(mask[j]&x)
			{
				x<<=1;
				if(foundZeros)
					pastZeros=true;
			}
			else
			{
				foundZeros=true;
				pastZeros=false;
				mask[j+1]|=x;
				x<<=1;
			}
		}
	}

	for(j=0;j<3;j++)
	{
		x=1;
		foundZeros=false;
		pastZeros=false;

		mask2[j+1]=mask2[j];

		while (x>0x100&&!pastZeros)
		{
			if(mask2[j]&x)
			{
				x<<=1;
				if(foundZeros)
					pastZeros=true;
			}
			else
			{
				foundZeros=true;
				pastZeros=false;
				mask2[j+1]|=x;
				x<<=1;
			}
		}
	}


    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
  }
	
    // Banks 30->3f and b0->bf, address ranges 6000->7fff is S-RAM.
    for (c = 0; c < 16; c++)
    {
		Map [0x306 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0x307 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0xb06 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0xb07 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		BlockIsRAM [0x306 + (c << 4)] = TRUE;
		BlockIsRAM [0x307 + (c << 4)] = TRUE;
		BlockIsRAM [0xb06 + (c << 4)] = TRUE;
		BlockIsRAM [0xb07 + (c << 4)] = TRUE;
    }

	for c=0; c<0x200; c+=16)
	{
		for(i=0;i<8;i++)
		{
			int e=3;
			int d=c>>4;
			while(d>mask[0])
			{
				d&=mask[e];
				e--;
			}

			int f=3;
			int g=c>>4;
			while(g>mask2[0])
			{
				g&=mask2[f];
				f--;
			}
			
			//stuff in HiROM areas
			Map[c+0x400+i]=&ROM[d*0x10000];
			Map[c+0xC00+i]=&ROM[d*0x10000];
			//MINI
			Map[c+0x600+i]=&ROMOffset1[g*0x10000];
			Map[c+0xE00+i]=&ROMOffset1[g*0x10000];

		}
		for(i=8;i<16;i++)
		{
			int e=3;
			int d=c>>4;
			while(d>mask[0])
			{
				d&=mask[e];
				e--;
			}

			int f=3;
			int g=c>>4;
			while(g>mask2[0])
			{
				g&=mask2[f];
				f--;
			}

	
			//all stuff
			//BASE
			Map[c+i]=&ROM[d*0x10000];
			Map[c+0x800+i]=&ROM[d*0x10000];
			Map[c+0x400+i]=&ROM[d*0x10000];
			Map[c+0xC00+i]=&ROM[d*0x10000];
			//MINI
			Map[c+0x200+i]=&ROMOffset1[g*0x10000];
			Map[c+0xA00+i]=&ROMOffset1[g*0x10000];
			Map[c+0x600+i]=&ROMOffset1[g*0x10000];
			Map[c+0xE00+i]=&ROMOffset1[g*0x10000];
		}

	}

	int bankmax=0x40+ (1<<(ROMSize-6));
	//safety for corrupt headers
	if(bankmax > 128)
		bankmax = 0x80;
	int sum=0;
	for(i=0x40;i<bankmax; i++)
	{
		uint8 * bank_low=(uint8*)Map[i<<4];
		for (c=0;c<0x10000; c++)
		{
			sum+=bank_low[c];
		}
	}
	CalculatedChecksum=sum&0xFFFF;

    MapRAM ();
    WriteProtectROM ();
}


//Untested!!
void CMemory::GNextROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) &Memory.FillRAM [0x3000] - 0x3000;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_BWRAM;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_BWRAM;
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	

    // Banks 40->4f (was 7f, but SNES docs and GNext overdumping shows nothing here.)
    for (c = 0; c < 0x100; c += 16)
    {
		for (i = c; i < c + 16; i++)
			Map [i + 0x400] = (uint8 *) &SRAM [(c << 12) & 0x1ffff];
		
		for (i = c; i < c + 16; i++)
		{
			BlockIsROM [i + 0x400] = FALSE;
		}
    }
	
    for (c = 0; c < 0x100; c += 16)
	{
		for (i = c; i < c + 16; i++)
			Map [i + 0x700] = (uint8 *) &ROMOffset1 [(c << 12) & (Slot1Size-1)];
	}

    // c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c;  i < c + 16; i++)
		{
			Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
    WriteProtectROM ();
	
    // Now copy the map and correct it for the SA1 CPU.
    memmove ((void *) SA1.WriteMap, (void *) WriteMap, sizeof (WriteMap));
    memmove ((void *) SA1.Map, (void *) Map, sizeof (Map));
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		SA1.Map [c + 0] = SA1.Map [c + 0x800] = &Memory.FillRAM [0x3000];
		SA1.Map [c + 1] = SA1.Map [c + 0x801] = (uint8 *) MAP_NONE;
		SA1.WriteMap [c + 0] = SA1.WriteMap [c + 0x800] = &Memory.FillRAM [0x3000];
		SA1.WriteMap [c + 1] = SA1.WriteMap [c + 0x801] = (uint8 *) MAP_NONE;
    }
	
    // Banks 60->6f
    for (c = 0; c < 0x100; c++)
		SA1.Map [c + 0x600] = SA1.WriteMap [c + 0x600] = (uint8 *) MAP_BWRAM_BITMAP;
    
    BWRAM = SRAM;
}

void CMemory::SufamiTurboAltROMMap ()
{
    int c;
    int i;
	
	if(Slot1Size!=0)
		Slot1SRAMSize=(1<<((uint8)ROMOffset1[0x32]))*1024;
	else Slot1Size=0x8000;
	if(Slot2Size!=0)
		Slot2SRAMSize=(1<<((uint8)ROMOffset2[0x32]))*1024;
else Slot2Size=0x8000;

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;

//		for (i = c + 8; i < c + 16; i++)
//		{
//			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
//			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
//		}
		
    }

	//Map Bios

	for (c=0; c<0x200; c+=16)
	{
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [((c>>4)*0x8000)%CalculatedSize] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}

	}
	

		for (c=0x200; c<0x400; c+=16)
		{
			for (i = c + 8; i < c + 16; i++)
			{
				if(Slot1Size!=0)
				{
					Map [i] = Map [i + 0x800] = &ROMOffset1 [(((c>>4)*0x8000)%Slot1Size)] - 0x8000;
					BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
				}
				else Map [i] = Map [i + 0x800] = (uint8*)MAP_NONE;
			}

		}

   for (c=0x400; c<0x600; c+=16)
	{
		for (i = c; i < c + 8; i++)
		{
			if(Slot2Size!=0)
			{
				Map [i] = Map [i + 0x800] = &ROMOffset2[(((c>>4)*0x8000)%Slot2Size)];
				BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
			}
			else Map [i] = Map [i + 0x800] = (uint8*)MAP_NONE;

		}
		for (i = c + 8; i < c + 16; i++)
		{
			if(Slot2Size!=0)
			{
				Map [i] = Map [i + 0x800] = &ROMOffset2[(((c>>4)*0x8000)%Slot2Size)] - 0x8000;
				BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
			}
			else Map [i] = Map [i + 0x800] = (uint8*)MAP_NONE;

		}

   }

    // Banks 60->67 (7F?), S-RAM
	if(Slot1SRAMSize!=0)
	{
	    for (c = 0; c < 0x100; c++)
		{
			Map [c + 0xE00] = Map [c + 0x600] = (uint8 *) MAP_LOROM_SRAM;
			BlockIsRAM [c + 0xE00] = BlockIsRAM [c + 0x600] = TRUE;
			BlockIsROM [c + 0xE00] = BlockIsROM [c + 0x600] = FALSE;
		}
    }
	if(Slot2SRAMSize!=0)
	{
	    for (c = 0; c < 0x100; c++)
		{
			Map [c + 0xF00] = Map [c + 0x700] = (uint8 *) MAP_LOROM_SRAM;
			BlockIsRAM [c + 0xF00] = BlockIsRAM [c + 0x700] = TRUE;
			BlockIsROM [c + 0xF00] = BlockIsROM [c + 0x700] = FALSE;
		}
    }
	
    // Banks 7e->7f, RAM
    for (c = 0; c < 16; c++)
    {
		Map [c + 0x7e0] = RAM;
		Map [c + 0x7f0] = RAM + 0x10000;
		BlockIsRAM [c + 0x7e0] = TRUE;
		BlockIsRAM [c + 0x7f0] = TRUE;
		BlockIsROM [c + 0x7e0] = FALSE;
		BlockIsROM [c + 0x7f0] = FALSE;
    }
	
    WriteProtectROM ();
}
#endif


void CMemory::SRAM512KLoROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [c << 11] - 0x8000;
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 8; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000];
		
		for (i = c + 8; i < c + 16; i++)
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 11) + 0x200000 - 0x8000];
		
		for (i = c; i < c + 16; i++)
		{
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
    MapExtraRAM ();
    WriteProtectROM ();
}
/*
void CMemory::BSHiROMMap ()
{
    int c;
    int i;
	
	SRAMSize=5;

    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		// XXX: How large is SRAM??
				Map [c + 5] = Map [c + 0x805] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
		BlockIsRAM [c + 5] = BlockIsRAM [c + 0x805] = TRUE;
//		Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_NONE;
//		Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_NONE;
		
						Map [c + 6] = Map [c + 0x806] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
BlockIsRAM [c + 6] = BlockIsRAM [c + 0x806] = TRUE;
		Map [c + 7] = Map [c + 0x807] = (uint8 *) RAM;
//		Map [c + 5] = Map [c + 0x805] = (uint8 *) SRAM;
BlockIsRAM [c + 7] = BlockIsRAM [c + 0x807] = TRUE;

		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 60->7d offset 0000->7fff & 60->7f offset 8000->ffff PSRAM
    // XXX: How large is PSRAM?

	//not adjusted, but The Dumper says "4 Mbits"
    for (c = 0x600; c < 0x7e0; c += 16)
    {
		for (i = c; i < c + 8; i++)
		{
			Map [i] = &ROM [0x400000 + (c << 11)];
			BlockIsRAM [i] = TRUE;
		}
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = &ROM [0x400000 + (c << 11) - 0x8000];
			BlockIsRAM [i] = TRUE;
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
		{
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	for(i=0;i<0x80;i++)
	{
		Map[0x700+i]=&BSRAM[0x10000*(i/16)];
		BlockIsRAM[0x700+i]=TRUE;
		BlockIsROM[0x700+i]=FALSE;
	}
	for (i=0; i<8;i++)
	{
		Map[0x205+(i<<4)]=Map[0x285+(i<<4)]=Map[0x305+(i<<4)]=Map[0x385+(i<<4)]=Map[0x705+(i<<4)];
		BlockIsRAM[0x205+(i<<4)]=BlockIsRAM[0x285+(i<<4)]=BlockIsRAM[0x305+(i<<4)]=BlockIsRAM[0x385+(i<<4)]=TRUE;
		BlockIsROM[0x205+(i<<4)]=BlockIsROM[0x285+(i<<4)]=BlockIsROM[0x305+(i<<4)]=BlockIsROM[0x385+(i<<4)]=FALSE;
	}

    MapRAM ();
    WriteProtectROM ();
}
*/
void CMemory::JumboLoROMMap (bool8 Interleaved)
{
    int c;
    int i;
	
	uint32 OFFSET0 = 0x400000;
    uint32 OFFSET1 = 0x400000;
    uint32 OFFSET2 = 0x000000;
	
    if (Interleaved)
    {
		OFFSET0 = 0x000000;
		OFFSET1 = 0x000000;
		OFFSET2 = CalculatedSize-0x400000; //changed to work with interleaved DKJM2.
    }
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		if (Settings.DSP1Master)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_DSP;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_DSP;
		}
		else if (Settings.C4)
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) MAP_C4;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) MAP_C4;
		}
		else
		{
			Map [c + 6] = Map [c + 0x806] = (uint8 *) bytes0x2000 - 0x6000;
			Map [c + 7] = Map [c + 0x807] = (uint8 *) bytes0x2000 - 0x6000;
		}
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i]= &ROM [((c << 11) % (CalculatedSize - 0x400000)) + OFFSET0] - 0x8000;
			Map [i + 0x800] = &ROM [((c << 11) % (0x400000)) + OFFSET2] - 0x8000;
			BlockIsROM [i + 0x800] = BlockIsROM [i] = TRUE;
		}
    }
	
    if (Settings.DSP1Master)
    {
		// Banks 30->3f and b0->bf
		for (c = 0x300; c < 0x400; c += 16)
		{
			for (i = c + 8; i < c + 16; i++)
			{
				Map [i + 0x800] = (uint8 *) MAP_DSP;
				BlockIsROM [i] = BlockIsROM [i + 0x800] = FALSE;
			}
		}
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0x400; c < 0x800; c += 16)
    {
		//updated mappings to correct A15 mirroring
		for (i = c; i < c + 8; i++)
		{
			Map [i]= &ROM [((c << 11) % (CalculatedSize - 0x400000)) + OFFSET0];
			Map [i + 0x800] = &ROM [((c << 11) % 0x400000) +OFFSET2];
		}

		for (i = c + 8; i < c + 16; i++)
		{
			Map [i]= &ROM [((c << 11) % (CalculatedSize - 0x400000)) + OFFSET0] - 0x8000;
			Map [i + 0x800] = &ROM [((c << 11) % 0x400000) + OFFSET2 ] - 0x8000;
		}
		
		for (i = c; i < c + 16; i++)	
		{
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }

	//ROM type has to be 64 Mbit header!
	int sum=0, k,l;
	for(k=0;k<256;k++)
	{
		uint8* bank=0x8000+Map[8+(k<<4)];//use upper half of the banks, and adjust for LoROM.
		for(l=0;l<0x8000;l++)
			sum+=bank[l];
	}
	CalculatedChecksum=sum&0xFFFF;

    MapRAM ();
    WriteProtectROM ();
}

uint32 CMemory::map_mirror (uint32 size, uint32 pos)
{
	// from bsnes
	if (size == 0)
		return (0);
	if (pos < size)
		return (pos);

	uint32	mask = 1 << 31;
	while (!(pos & mask))
		mask >>= 1;

	if (size <= (pos & mask))
		return (map_mirror(size, pos - mask));
	else
		return (mask + map_mirror(size - mask, pos - mask));
}

void CMemory::map_hirom_offset (uint32 bank_s, uint32 bank_e, uint32 addr_s, uint32 addr_e, uint32 size, uint32 offset)
{
	uint32	c, i, p, addr;

	for (c = bank_s; c <= bank_e; c++)
	{
		for (i = addr_s; i <= addr_e; i += 0x1000)
		{
			p = (c << 4) | (i >> 12);
			addr = (c - bank_s) << 16;
			Map[p] = ROM + offset + map_mirror(size, addr);
			BlockIsROM[p] = TRUE;
			BlockIsRAM[p] = FALSE;
		}
	}
}

void CMemory::SPC7110HiROMMap ()
{
    int c;
    int i;
	
    // Banks 00->3f and 80->bf
    for (c = 0; c < 0x400; c += 16)
    {
		Map [c + 0] = Map [c + 0x800] = RAM;
		BlockIsRAM [c + 0] = BlockIsRAM [c + 0x800] = TRUE;
		Map [c + 1] = Map [c + 0x801] = RAM;
		BlockIsRAM [c + 1] = BlockIsRAM [c + 0x801] = TRUE;
		
		Map [c + 2] = Map [c + 0x802] = (uint8 *) MAP_PPU;
		Map [c + 3] = Map [c + 0x803] = (uint8 *) MAP_PPU;
		Map [c + 4] = Map [c + 0x804] = (uint8 *) MAP_CPU;
		Map [c + 5] = Map [c + 0x805] = (uint8 *) MAP_CPU;
		
		Map [c + 6] /*= Map [c + 0x806]*/ = (uint8 *) MAP_HIROM_SRAM;
		Map [c + 7] /*= Map [c + 0x807]*/ = (uint8 *) MAP_HIROM_SRAM;
		Map [c + 0x806]=Map [c + 0x807]= (uint8 *) MAP_NONE;
		
		for (i = c + 8; i < c + 16; i++)
		{
			Map [i] = Map [i + 0x800] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i] = BlockIsROM [i + 0x800] = TRUE;
		}
    }
	
    // Banks 30->3f and b0->bf, address ranges 6000->7fff is S-RAM.
    for (c = 0; c < 16; c++)
    {
		Map [0x306 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0x307 + (c << 4)] = (uint8 *) MAP_HIROM_SRAM;
		Map [0xb06 + (c << 4)] = (uint8 *) MAP_NONE;
		Map [0xb07 + (c << 4)] = (uint8 *) MAP_NONE;
		BlockIsRAM [0x306 + (c << 4)] = TRUE;
		BlockIsRAM [0x307 + (c << 4)] = TRUE;
		//	BlockIsRAM [0xb06 + (c << 4)] = TRUE;
		//	BlockIsRAM [0xb07 + (c << 4)] = TRUE;
    }
	
    // Banks 40->7f and c0->ff
    for (c = 0; c < 0x400; c += 16)
    {
		for (i = c; i < c + 16; i++)
		{
			Map [i + 0x400] = Map [i + 0xc00] = &ROM [(c << 12) % CalculatedSize];
			BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = TRUE;
		}
    }
	
	for (c=0;c<0x10;c++)
	{
		Map [0x500+c]=(uint8 *)MAP_SPC7110_DRAM;
		BlockIsROM [0x500+c]=TRUE;
	}
	
	for (c=0;c<0x100;c++)
	{
		Map [0xD00+c] = (uint8 *) MAP_SPC7110_ROM;
		Map [0xE00+c] = (uint8 *) MAP_SPC7110_ROM;
		Map [0xF00+c] = (uint8 *) MAP_SPC7110_ROM;
		BlockIsROM [0xD00+c] = BlockIsROM [0xE00+c] = BlockIsROM [0xF00+c] = TRUE;
		
	}

	// For Tengai Makyou (English) 
	//if (ROMCRC32 == 0xE589FB4)
	if (strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21) == 0 && CalculatedSize > 5242880)
	{
		map_hirom_offset(0x40, 0x4f, 0x0000, 0xffff, CalculatedSize, 0x600000);
	}

	S9xSpc7110Init();

int sum=0;
for(i=0;i<(int)CalculatedSize; i++)
{
	sum+=ROM[i];
}

if(CalculatedSize==0x300000)
	sum<<=1;
CalculatedChecksum=sum&0xFFFF;

    MapRAM ();
    WriteProtectROM ();
}
void CMemory::SPC7110Sram(uint8 newstate)
{
	if(newstate&0x80)
	{
		Memory.Map[6]=(uint8 *)MAP_HIROM_SRAM;
		Memory.Map[7]=(uint8 *)MAP_HIROM_SRAM;
		Memory.Map[0x306]=(uint8 *)MAP_HIROM_SRAM;
		Memory.Map[0x307]=(uint8 *)MAP_HIROM_SRAM;
		
		
	}
	else
	{
		Memory.Map[6]=(uint8 *)MAP_RONLY_SRAM;
		Memory.Map[7]=(uint8 *)MAP_RONLY_SRAM;
		Memory.Map[0x306]=(uint8 *)MAP_RONLY_SRAM;
		Memory.Map[0x307]=(uint8 *)MAP_RONLY_SRAM;
	}
}
const char *CMemory::TVStandard ()
{
    return (Settings.PAL ? "PAL" : "NTSC");
}

const char *CMemory::Speed ()
{
    return (ROMSpeed & 0x10 ? "120ns" : "200ns");
}

const char *CMemory::MapType ()
{
    return (HiROM ? "HiROM" : "LoROM");
}

const char *CMemory::StaticRAMSize ()
{
    static char tmp [20];
	
    if (Memory.SRAMSize > 16)
		return ("Corrupt");
    sprintf (tmp, "%dKB", (SRAMMask + 1) / 1024);
    return (tmp);
}

const char *CMemory::Size ()
{
    static char tmp [20];
	
    if (ROMSize < 7 || ROMSize - 7 > 23)
		return ("Corrupt");
    sprintf (tmp, "%dMbits", 1 << (ROMSize - 7));
    return (tmp);
}

const char *CMemory::KartContents ()
{
    static char tmp [30];
    static const char *CoPro [16] = {
		"DSP", "SuperFX", "OBC1", "SA-1", "S-DD1", "S-RTC", "CoPro#6",
			"CoPro#7", "CoPro#8", "CoPro#9", "CoPro#10", "CoPro#11", "CoPro#12",
			"CoPro#13", "CoPro#14", "CoPro-Custom"
    };
    static const char *Contents [3] = {
		"ROM", "ROM+RAM", "ROM+RAM+BAT"
    };
	static const char *DSPSel [4] = {
		"DSP1", "DSP2", "DSP3", "DSP4"
	};
    if (ROMType == 0&&!Settings.BS)
		return ("ROM only");
	
    sprintf (tmp, "%s", Contents [(ROMType & 0xf) % 3]);
	
	if(Settings.BS)
		sprintf (tmp, "%s+%s", tmp, "BSX");
	else if(Settings.SPC7110&&Settings.SPC7110RTC)
		sprintf (tmp, "%s+%s", tmp, "SPC7110+RTC");
	else if(Settings.SPC7110)
		sprintf (tmp, "%s+%s", tmp, "SPC7110");
	else if(Settings.C4)
		sprintf (tmp, "%s+%s", tmp, "C4");
	else if(Settings.SETA!=0)
	{
		switch(Settings.SETA)
		{
		case ST_010:
			sprintf (tmp, "%s+%s", tmp, "ST-010");
			break;
		case ST_011:
			sprintf (tmp, "%s+%s", tmp, "ST-011");
			break;

		case ST_018:
			sprintf (tmp, "%s+%s", tmp, "ST-018");
			break;

		}
	}
    else if ((ROMType & 0xf) >= 3)
	{
		if (ROMType & 0xf0) 
			sprintf (tmp, "%s+%s", tmp, CoPro [(ROMType & 0xf0) >> 4]);
		else
			sprintf (tmp, "%s+%s", tmp, DSPSel [Settings.DSP-1]);
	}
	
    return (tmp);
}

const char *CMemory::MapMode ()
{
    static char tmp [4];
    sprintf (tmp, "%02x", ROMSpeed & ~0x10);
    return (tmp);
}

const char *CMemory::ROMID ()
{
    return (ROMId);
}


// Applies a speed hack at the given the PB:PC location.
// It replaces the first byte with the WDM (0x42) opcode.
//
bool CMemory::SpeedHackAdd(int address, int cyclesPerSkip, int16 originalByte1, int16 originalByte2, int16 originalByte3, int16 originalByte4)
{
	if (SNESGameFixes.SpeedHackCount >= 8)
		return false;

	// Get the actual location of the memory to patch
	//
    int block;
    uint8 *GetAddress = Memory.Map [block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK];
	uint8 *finalAddress = 0;
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		finalAddress = GetAddress + (address & 0xffff);
	if (finalAddress == 0)
		return false;
	
	SNESGameFixes.SpeedHackOriginalBytes[SNESGameFixes.SpeedHackCount][0] = originalByte1;
	SNESGameFixes.SpeedHackOriginalBytes[SNESGameFixes.SpeedHackCount][1] = originalByte2;
	SNESGameFixes.SpeedHackOriginalBytes[SNESGameFixes.SpeedHackCount][2] = originalByte3;
	SNESGameFixes.SpeedHackOriginalBytes[SNESGameFixes.SpeedHackCount][3] = originalByte4;
	
	SNESGameFixes.SpeedHackSNESAddress[SNESGameFixes.SpeedHackCount] = (uint32) address;
	SNESGameFixes.SpeedHackAddress[SNESGameFixes.SpeedHackCount] = finalAddress;
	SNESGameFixes.SpeedHackOriginalOpcode[SNESGameFixes.SpeedHackCount] = originalByte1;
	SNESGameFixes.SpeedHackCycles[SNESGameFixes.SpeedHackCount] = cyclesPerSkip;
	SNESGameFixes.SpeedHackCount++;

	return true;
}

// Applies a speed hack at the given the PB:PC location.
// It replaces the first byte with the WDM (0x42) opcode.
//
bool CMemory::SpeedHackSA1Add(int address, int16 originalByte1, int16 originalByte2, int16 originalByte3, int16 originalByte4)
{
	if (SNESGameFixes.SpeedHackSA1Count >= 8)
		return false;

	// Get the actual location of the memory to patch
	//
    int block;
    uint8 *GetAddress = SA1.Map [block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK];
	uint8 *finalAddress = 0;
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		finalAddress = GetAddress + (address & 0xffff);
	if (finalAddress == 0)
		return false;
	
	SNESGameFixes.SpeedHackSA1OriginalBytes[SNESGameFixes.SpeedHackSA1Count][0] = originalByte1;
	SNESGameFixes.SpeedHackSA1OriginalBytes[SNESGameFixes.SpeedHackSA1Count][1] = originalByte2;
	SNESGameFixes.SpeedHackSA1OriginalBytes[SNESGameFixes.SpeedHackSA1Count][2] = originalByte3;
	SNESGameFixes.SpeedHackSA1OriginalBytes[SNESGameFixes.SpeedHackSA1Count][3] = originalByte4;
	
	SNESGameFixes.SpeedHackSA1SNESAddress[SNESGameFixes.SpeedHackSA1Count] = (uint32) address;
	SNESGameFixes.SpeedHackSA1Address[SNESGameFixes.SpeedHackSA1Count] = finalAddress;
	SNESGameFixes.SpeedHackSA1OriginalOpcode[SNESGameFixes.SpeedHackSA1Count] = originalByte1;
	SNESGameFixes.SpeedHackSA1Count++;
	
	return true;
}


// This fixes a critical bug because originally, ApplySpeedHackPatches
// calls S9xGetByte, which increments the cycles counter unnecessarily!
//
uint8 CMemory::GetByte (uint32 Address)
{
    int block;
    uint8 *GetAddress = CPU.MemoryMap [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (*(GetAddress + (Address & 0xffff)));
	else 
		return S9xGetByteFromRegister(GetAddress, Address);
}

void CMemory::ApplySpeedHackPatches()
{
	// Patch 
	for (int n = 0; n < SNESGameFixes.SpeedHackCount; n++)
	{
		// First check that the original bytes matches.
		//
		bool allMatches = true;
		for (int i = 0; i < 4 && SNESGameFixes.SpeedHackOriginalBytes[n][i] != -1; i++)
		{
			uint8 byte = GetByte(SNESGameFixes.SpeedHackSNESAddress[n] + i);
			if (SNESGameFixes.SpeedHackOriginalBytes[n][i] != byte)
			{
				allMatches = false;
				break;
			}
		}

		if (allMatches)
		{
			*SNESGameFixes.SpeedHackAddress[n] = 0x42;
			//printf ("Patched main: %x\n", SNESGameFixes.SpeedHackSNESAddress[n]);
		}			
	}

	for (int n = 0; n < SNESGameFixes.SpeedHackSA1Count; n++)
	{
		// First check that the original bytes matches.
		//
		bool allMatches = true;
		for (int i = 0; i < 4 && SNESGameFixes.SpeedHackSA1OriginalBytes[n][i] != -1; i++)
		{
			uint8 byte = GetByte(SNESGameFixes.SpeedHackSA1SNESAddress[n] + i);
			if (SNESGameFixes.SpeedHackSA1OriginalBytes[n][i] != byte)
			{
				allMatches = false;
				break;
			}
		}

		if (allMatches)
		{
			*SNESGameFixes.SpeedHackSA1Address[n] = 0x42;
			//printf ("Patched main: %x\n", SNESGameFixes.SpeedHackSA1SNESAddress[n]);
		}
	}

}

void CMemory::ApplyROMFixes ()
{
#ifdef __W32_HEAP
	if(_HEAPOK!=_heapchk())
		MessageBox(GUI.hWnd, "CMemory::ApplyROMFixes", "Heap Corrupt", MB_OK);
#endif

	//don't steal my work! -MK
	if(ROMCRC32 == 0x1B4A5616 && strncmp(ROMName, "RUDORA NO HIHOU", 15)==0)
		strncpy(ROMName, "THIS SCRIPT WAS STOLEN", 22);

	/*
	HACKS NSRT can fix that we hadn't detected before.
[14:25:13] <@Nach>     case 0x0c572ef0: //So called Hook (US)(2648)
[14:25:13] <@Nach>     case 0x6810aa95: //Bazooka Blitzkreig swapped sizes hack -handled
[14:25:17] <@Nach>     case 0x61E29C06: //The Tick region hack
[14:25:19] <@Nach>     case 0x1EF90F74: //Jikkyou Keiba Simulation Stable Star PAL hack
[14:25:23] <@Nach>     case 0x4ab225b5: //So called Krusty's Super Fun House (E)
[14:25:25] <@Nach>     case 0x77fd806a: //Donkey Kong Country 2 (E) v1.1 bad dump -handled
[14:25:27] <@Nach>     case 0x340f23e5: //Donkey Kong Country 3 (U) copier hack - handled
	*/


	//Ambiguous chip function pointer assignments
	//DSP1.version=Settings.DSP-1;


	if(strncmp(ROMName, "SD\x0b6\x0de\x0dd\x0c0\x0de\x0d1GX", 10)==0)
	{
		strncpy(ROMName, "SD Gundam GX", 13);
		DSP3_Reset();
	}


	//memory map corrections
	if(strncmp(ROMName, "XBAND",5)==0)
	{
		for (int c=0xE00;c<0xE10;c++)
		{
			Map [c] = (uint8 *) MAP_LOROM_SRAM;
			BlockIsRAM [c] = TRUE;
			BlockIsROM [c] = FALSE;
		}
		WriteProtectROM ();
	}

		//not MAD-1 compliant
	if(strcmp (ROMName, "WANDERERS FROM YS") == 0)
	{
		for(int c=0;c<0xE0;c++)
		{
			Map[c+0x700]=(uint8*)MAP_LOROM_SRAM;
			BlockIsROM[c+0x700]=FALSE;
			BlockIsRAM[c+0x700]=TRUE;
		}
		WriteProtectROM();
	}

    if (strcmp (ROMName, "GOGO ACKMAN3") == 0 || 
		strcmp (ROMName, "HOME ALONE") == 0)
    {
		// Banks 00->3f and 80->bf
		for (int c = 0; c < 0x400; c += 16)
		{
			Map [c + 6] = Map [c + 0x806] = SRAM;
			Map [c + 7] = Map [c + 0x807] = SRAM;
			BlockIsROM [c + 6] = BlockIsROM [c + 0x806] = FALSE;
			BlockIsROM [c + 7] = BlockIsROM [c + 0x807] = FALSE;
			BlockIsRAM [c + 6] = BlockIsRAM [c + 0x806] = TRUE;
			BlockIsRAM [c + 7] = BlockIsRAM [c + 0x807] = TRUE;
		}
		WriteProtectROM ();
    }

	if (strcmp (ROMName, "RADICAL DREAMERS") == 0 ||
		strcmp (ROMName, "TREASURE CONFLIX") == 0)
    {
		int c;
		
		for (c = 0; c < 0x80; c++)
		{
			Map [c + 0x700] = ROM + 0x200000 + 0x1000 * (c & 0xf0);
			BlockIsRAM [c + 0x700] = TRUE;
			BlockIsROM [c + 0x700] = FALSE;
		}
		for (c = 0; c < 0x400; c += 16)
		{
			Map [c + 5] = Map [c + 0x805] = ROM + 0x300000;
			BlockIsRAM [c + 5] = BlockIsRAM [c + 0x805] = TRUE;
		}
		WriteProtectROM ();
    }

	if(strncmp(ROMName, "WAR 2410", 8)==0)
	{
		Map [0x005] = (uint8 *) RAM;
		BlockIsRAM [0x005] = TRUE;
		BlockIsROM [0x005] = FALSE;
	}

    if (strcmp (ROMName, "BATMAN--REVENGE JOKER") == 0)
    {
		Memory.HiROM = FALSE;
		Memory.LoROM = TRUE;
		LoROMMap ();
    }


	//NMI hacks
    CPU.NMITriggerPoint = 4;
    if (strcmp (ROMName, "CACOMA KNIGHT") == 0)
		CPU.NMITriggerPoint = 25;
		
	//Disabling a speed-up
    // Games which spool sound samples between the SNES and sound CPU using
    // H-DMA as the sample is playing.
    if (strcmp (ROMName, "EARTHWORM JIM 2") == 0 ||
		strcmp (ROMName, "PRIMAL RAGE") == 0 ||
		strcmp (ROMName, "CLAY FIGHTER") == 0 ||
		strcmp (ROMName, "ClayFighter 2") == 0 ||
		strncasecmp (ROMName, "MADDEN", 6) == 0 ||
		strncmp (ROMName, "NHL", 3) == 0 ||
		strcmp (ROMName, "WeaponLord") == 0||
		strncmp(ROMName, "WAR 2410", 8)==0)
    {
		Settings.Shutdown = FALSE;
    }
	

	//APU timing hacks
	
    // Stunt Racer FX
    if (strcmp (ROMId, "CQ  ") == 0 ||
		// Illusion of Gaia
        strncmp (ROMId, "JG", 2) == 0 ||
		strcmp (ROMName, "GAIA GENSOUKI 1 JPN") == 0)
    {
		IAPU.OneCycle = 13;
    }
	
    // RENDERING RANGER R2
    if (strcmp (ROMId, "AVCJ") == 0 ||
		//Mark Davis
		strncmp(ROMName, "THE FISHING MASTER", 18)==0 || //needs >= actual APU timing. (21 is .002 Mhz slower)
		// Star Ocean
		strncmp (ROMId, "ARF", 3) == 0 ||
		// Tales of Phantasia
		strncmp (ROMId, "ATV", 3) == 0 ||
		// Act Raiser 1 & 2
		strncasecmp (ROMName, "ActRaiser", 9) == 0 ||
		// Soulblazer
		strcmp (ROMName, "SOULBLAZER - 1 USA") == 0 ||
		strcmp (ROMName, "SOULBLADER - 1") == 0 ||

		// Terranigma
		strncmp (ROMId, "AQT", 3) == 0 ||
		// Robotrek
		strncmp (ROMId, "E9 ", 3) == 0 ||
		strcmp (ROMName, "SLAP STICK 1 JPN") == 0 ||
		// ZENNIHON PURORESU2
		strncmp (ROMId, "APR", 3) == 0 ||
		// Bomberman 4
		strncmp (ROMId, "A4B", 3) == 0 ||
		// UFO KAMEN YAKISOBAN
		strncmp (ROMId, "Y7 ", 3) == 0 ||
		strncmp (ROMId, "Y9 ", 3) == 0 ||
		// Panic Bomber World
		strncmp (ROMId, "APB", 3) == 0 ||
		((strncmp (ROMName, "Parlor", 6) == 0 || 
		strcmp (ROMName, "HEIWA Parlor!Mini8") == 0 ||
		strncmp (ROMName, "SANKYO Fever! \xCC\xA8\xB0\xCA\xDE\xB0!", 21) == 0) && //SANKYO Fever! Fever!
		strcmp (CompanyId, "A0") == 0) ||
		strcmp (ROMName, "DARK KINGDOM") == 0 ||
		strcmp (ROMName, "ZAN3 SFC") == 0 ||
		strcmp (ROMName, "HIOUDEN") == 0 ||
		strcmp (ROMName, "\xC3\xDD\xBC\xC9\xB3\xC0") == 0 ||  //Tenshi no Uta
		strcmp (ROMName, "FORTUNE QUEST") == 0 ||
		strcmp (ROMName, "FISHING TO BASSING") == 0 ||
		strncmp (ROMName, "TokyoDome '95Battle 7", 21) == 0 ||
		strcmp (ROMName, "OHMONO BLACKBASS") == 0 ||
		strncmp (ROMName, "SWORD WORLD SFC", 15) == 0 ||
		strcmp (ROMName, "MASTERS") ==0 || //Augusta 2 J
		strcmp (ROMName, "SFC \xB6\xD2\xDD\xD7\xB2\xC0\xDE\xB0") == 0 || //Kamen Rider
		strncmp (ROMName, "LETs PACHINKO(", 14) == 0) //A set of BS games
    {
		IAPU.OneCycle = 15;
    }
    

	//Specific game fixes

	Settings.StarfoxHack = strcmp (ROMName, "STAR FOX") == 0 || strcmp (ROMName, "STAR WING") == 0;
	Settings.WinterGold = strcmp (ROMName, "FX SKIING NINTENDO 96") == 0 || strcmp (ROMName, "DIRT RACER") == 0 || Settings.StarfoxHack;
	

	if((strcmp(ROMName, "LEGEND")==0&&!Settings.PAL)|| strcmp(ROMName, "King Arthurs World")==0)
		SNESGameFixes.EchoOnlyOutput=TRUE;


	    Settings.DaffyDuck = (strcmp (ROMName, "DAFFY DUCK: MARV MISS") == 0) ||
		(strcmp (ROMName, "ROBOCOP VS THE TERMIN") == 0) ||
		(strcmp (ROMName, "ROBOCOP VS TERMINATOR") == 0); //ROBOCOP VS THE TERMIN
    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
	
	//OAM hacks because we don't fully understand the
	//behavior of the SNES.

	//Totally wacky display...
	//seems to need a disproven behavior, so
	//we're definitely overlooking some other bug?
	if(strncmp(ROMName, "UNIRACERS", 9)==0)
		SNESGameFixes.Uniracers=true;


	//is this even useful now?
    if (strcmp (ROMName, "ALIENS vs. PREDATOR") == 0)
		SNESGameFixes.alienVSpredetorFix = TRUE;

	// Fixes CuOnPa
    if (strcmp (ROMId, "AC6J") == 0)
		SNESGameFixes.cuonpaFix = TRUE;
		
    if (strcmp (ROMName, "\xBD\xB0\xCA\xDF\xB0\xCC\xA7\xD0\xBD\xC0") == 0 ||  //Super Famista
		strcmp (ROMName, "\xBD\xB0\xCA\xDF\xB0\xCC\xA7\xD0\xBD\xC0 2") == 0 || //Super Famista 2
		strcmp (ROMName, "ZENKI TENCHIMEIDOU") == 0 ||
		strcmp (ROMName, "GANBA LEAGUE") == 0)
    {
		SNESGameFixes.APU_OutPorts_ReturnValueFix = TRUE;
    }

    if (strcmp (ROMName, "FURAI NO SIREN") == 0)
		SNESGameFixes.SoundEnvelopeHeightReading2 = TRUE;

	//CPU timing hacks
	    Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 
		      Settings.CyclesPercentage) / 100;

		//no need to ifdef for right now...
//#ifdef HDMA_HACKS

	// A Couple of HDMA related hacks - Lantus
	if ((strcmp(ROMName, "SFX SUPERBUTOUDEN2")==0) ||
	    (strcmp(ROMName, "ALIEN vs. PREDATOR")==0) ||
		(strcmp(ROMName, "STONE PROTECTORS")==0) ||
	    (strcmp(ROMName, "SUPER BATTLETANK 2")==0))
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 130) / 100;

	if(strcmp(ROMName, "HOME IMPROVEMENT")==0)
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 200) / 100;
 
	// End HDMA hacks
//#endif

	if(strcmp(ROMName, "ActRaiser-2 USA")==0)
			Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 83) / 100;

	
    if (strcmp (ROMId, "ASRJ") == 0 && Settings.CyclesPercentage == 100)
		// Street Racer
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 95) / 100;

	// Power Rangers Fight
    if (strncmp (ROMId, "A3R", 3) == 0 ||
        // Clock Tower
		strncmp (ROMId, "AJE", 3) == 0)
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 103) / 100;
    
	
    if (strncmp (ROMId, "A3M", 3) == 0 && Settings.CyclesPercentage == 100)
		// Mortal Kombat 3. Fixes cut off speech sample
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 110) / 100;
	
	//Darkness Beyond Twilight
	//Crimson beyond blood that flows
	//buried in the stream of time
	//is where your power grows
	//I pledge myself to conquer
	//all the foes who stand
	//before the might gift betsowed
	//in my unworthy hand
    if (strcmp (ROMName, "\x0bd\x0da\x0b2\x0d4\x0b0\x0bd\x0de") == 0 &&
		Settings.CyclesPercentage == 100)
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 101) / 100;


#ifdef DETECT_NASTY_FX_INTERLEAVE
//XXX: Test without these. Win32 port indicates they aren't needed?	
//Apparently are needed!
    if (strcmp (ROMName, "WILD TRAX") == 0 || 
		strcmp (ROMName, "STAR FOX 2") == 0 || 
		strcmp (ROMName, "YOSSY'S ISLAND") == 0 || 
		strcmp (ROMName, "YOSHI'S ISLAND") == 0)
		CPU.TriedInterleavedMode2 = TRUE;
#endif

    // Start Trek: Deep Sleep 9
    if (strncmp (ROMId, "A9D", 3) == 0 && Settings.CyclesPercentage == 100)
		Settings.H_Max = (SNES_CYCLES_PER_SCANLINE * 110) / 100;


	//SA-1 Speedup settings
    SA1.WaitAddress = NULL;
    SA1.WaitByteAddress1 = NULL;
    SA1.WaitByteAddress2 = NULL;
	
	if (Settings.SA1)
   {
		/* Bass Fishing */
		if (strcmp (ROMId, "ZBPJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0093f1 >> MEMMAP_SHIFT] + 0x93f1;
			SA1.WaitByteAddress1 = FillRAM + 0x304a;
		}
		/* DAISENRYAKU EXPERTWW2 */
		if (strcmp (ROMId, "AEVJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0ed18d >> MEMMAP_SHIFT] + 0xd18d;
			SA1.WaitByteAddress1 = FillRAM + 0x3000;
		}
		/* debjk2 */
		if (strcmp (ROMId, "A2DJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x008b62 >> MEMMAP_SHIFT] + 0x8b62;
		}
		/* Dragon Ballz HD */
		if (strcmp (ROMId, "AZIJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x008083 >> MEMMAP_SHIFT] + 0x8083;
			SA1.WaitByteAddress1 = FillRAM + 0x3020;
		}
		/* SFC SDGUNDAMGNEXT */
		if (strcmp (ROMId, "ZX3J") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0087f2 >> MEMMAP_SHIFT] + 0x87f2;
			SA1.WaitByteAddress1 = FillRAM + 0x30c4;
			SA1.WaitByteAddress1 = FillRAM + 0x30b0;
		}
		/* ShougiNoHanamichi */
		if (strcmp (ROMId, "AARJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc1f85a >> MEMMAP_SHIFT] + 0xf85a;
			SA1.WaitByteAddress1 = SRAM + 0x0c64;
			SA1.WaitByteAddress2 = SRAM + 0x0c66;
		}
		/* KATO HIFUMI9DAN SYOGI */
		if (strcmp (ROMId, "A23J") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc25037 >> MEMMAP_SHIFT] + 0x5037;
			SA1.WaitByteAddress1 = SRAM + 0x0c06;
			SA1.WaitByteAddress2 = SRAM + 0x0c08;
		}
		/* idaten */
		if (strcmp (ROMId, "AIIJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc100be >> MEMMAP_SHIFT] + 0x00be;
			SA1.WaitByteAddress1 = SRAM + 0x1002;
			SA1.WaitByteAddress2 = SRAM + 0x1004;
		}
		/* igotais */
		if (strcmp (ROMId, "AITJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0080b7 >> MEMMAP_SHIFT] + 0x80b7;
		}
		/* J96 DREAM STADIUM */
		if (strcmp (ROMId, "AJ6J") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc0f74a >> MEMMAP_SHIFT] + 0xf74a;
		}
		/* JumpinDerby */
		if (strcmp (ROMId, "AJUJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00d926 >> MEMMAP_SHIFT] + 0xd926;
		}
		/* JKAKINOKI SHOUGI */
		if (strcmp (ROMId, "AKAJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00f070 >> MEMMAP_SHIFT] + 0xf070;
		}
		/* HOSHI NO KIRBY 3 & KIRBY'S DREAM LAND 3 JAP & US */
		if (strcmp (ROMId, "AFJJ") == 0 || strcmp (ROMId, "AFJE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0082d4 >> MEMMAP_SHIFT] + 0x82d4;
			SA1.WaitByteAddress1 = SRAM + 0x72a4;
		}
		/* KIRBY SUPER DELUXE JAP */
		if (strcmp (ROMId, "AKFJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x008c93 >> MEMMAP_SHIFT] + 0x8c93;
			SA1.WaitByteAddress1 = FillRAM + 0x300a;
			SA1.WaitByteAddress2 = FillRAM + 0x300e;
		}
		/* KIRBY SUPER DELUXE US */
		if (strcmp (ROMId, "AKFE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x008cb8 >> MEMMAP_SHIFT] + 0x8cb8;
			SA1.WaitByteAddress1 = FillRAM + 0x300a;
			SA1.WaitByteAddress2 = FillRAM + 0x300e;
		}
		/* SUPER MARIO RPG JAP & US */
		if (strcmp (ROMId, "ARWJ") == 0 || strcmp (ROMId, "ARWE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc0816f >> MEMMAP_SHIFT] + 0x816f;
			SA1.WaitByteAddress1 = FillRAM + 0x3000;
		}
		/* marvelous.zip */
		if (strcmp (ROMId, "AVRJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0085f2 >> MEMMAP_SHIFT] + 0x85f2;
			SA1.WaitByteAddress1 = FillRAM + 0x3024;
		}
		/* AUGUSTA3 MASTERS NEW */
		if (strcmp (ROMId, "AO3J") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00dddb >> MEMMAP_SHIFT] + 0xdddb;
			SA1.WaitByteAddress1 = FillRAM + 0x37b4;
		}
		/* OSHABERI PARODIUS */
		if (strcmp (ROMId, "AJOJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x8084e5 >> MEMMAP_SHIFT] + 0x84e5;
		}
		/* PANIC BOMBER WORLD */
		if (strcmp (ROMId, "APBJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00857a >> MEMMAP_SHIFT] + 0x857a;
		}
		/* PEBBLE BEACH NEW */
		if (strcmp (ROMId, "AONJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00df33 >> MEMMAP_SHIFT] + 0xdf33;
			SA1.WaitByteAddress1 = FillRAM + 0x37b4;
		}
		/* PGA EUROPEAN TOUR */
		if (strcmp (ROMId, "AEPE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x003700 >> MEMMAP_SHIFT] + 0x3700;
			SA1.WaitByteAddress1 = FillRAM + 0x3102;
		}
		/* PGA TOUR 96 */
		if (strcmp (ROMId, "A3GE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x003700 >> MEMMAP_SHIFT] + 0x3700;
			SA1.WaitByteAddress1 = FillRAM + 0x3102;
		}
		/* POWER RANGERS 4 */
		if (strcmp (ROMId, "A4RE") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x009899 >> MEMMAP_SHIFT] + 0x9899;
			SA1.WaitByteAddress1 = FillRAM + 0x3000;
		}
		/* PACHISURO PALUSUPE */
		if (strcmp (ROMId, "AGFJ") == 0)
		{
			// Never seems to turn on the SA-1!
		}
		/* SD F1 GRAND PRIX */
		if (strcmp (ROMId, "AGFJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x0181bc >> MEMMAP_SHIFT] + 0x81bc;
		}
		/* SHOUGI MARJONG */
		if (strcmp (ROMId, "ASYJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00f2cc >> MEMMAP_SHIFT] + 0xf2cc;
			SA1.WaitByteAddress1 = SRAM + 0x7ffe;
			SA1.WaitByteAddress2 = SRAM + 0x7ffc;
		}
		/* shogisai2 */
		if (strcmp (ROMId, "AX2J") == 0)
		{
			SA1.WaitAddress = SA1.Map [0x00d675 >> MEMMAP_SHIFT] + 0xd675;
		}
		
		/* SHINING SCORPION */
		if (strcmp (ROMId, "A4WJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc048be >> MEMMAP_SHIFT] + 0x48be;
		}
		/* SHIN SHOUGI CLUB */
		if (strcmp (ROMId, "AHJJ") == 0)
		{
			SA1.WaitAddress = SA1.Map [0xc1002a >> MEMMAP_SHIFT] + 0x002a;
			SA1.WaitByteAddress1 = SRAM + 0x0806;
			SA1.WaitByteAddress2 = SRAM + 0x0808;
		}
   }

	//Other

    // Additional game fixes by sanmaiwashi ...
    if (strcmp (ROMName, "SFX \xC5\xB2\xC4\xB6\xDE\xDD\xC0\xDE\xD1\xD3\xC9\xB6\xDE\xC0\xD8 1") == 0) // Gundam Knight Story
    {
		bytes0x2000 [0xb18] = 0x4c;
		bytes0x2000 [0xb19] = 0x4b;
		bytes0x2000 [0xb1a] = 0xea;
		SNESGameFixes.SRAMInitialValue = 0x6b;
    }
	
	
    // HITOMI3
    if (strcmp (ROMName, "HITOMI3") == 0)
    {
		Memory.SRAMSize = 1;
		SRAMMask = Memory.SRAMSize ?
			((1 << (Memory.SRAMSize + 3)) * 128) - 1 : 0;
    }

	//for (int i = 0; i < 1000; i++)
	//printf ("SRAM Size: %d %x\n", Memory.SRAMSize, SRAMMask);

	//sram value fixes
    if (strcmp (Memory.ROMName, "SUPER DRIFT OUT") == 0 ||
		strcmp(Memory.ROMName, "SATAN IS OUR FATHER!") == 0 ||
		strcmp (ROMName, "goemon 4") == 0)
		SNESGameFixes.SRAMInitialValue = 0x00;

#if 0
    if(strcmp (ROMName, "XBAND JAPANESE MODEM") == 0)
    {
		for (c = 0x200; c < 0x400; c += 16)
		{
			for (int i = c; i < c + 16; i++)
			{
				Map [i + 0x400] = Map [i + 0xc00] = &ROM[c * 0x1000];
				BlockIsRAM [i + 0x400] = BlockIsRAM [i + 0xc00] = TRUE;
				BlockIsROM [i + 0x400] = BlockIsROM [i + 0xc00] = FALSE;
			}
		}
		WriteProtectROM ();
    }
#endif
	
#define RomPatch(adr,ov,nv) \
	if (ROM [adr] == ov) \
    ROM [adr] = nv
	
    // Love Quest
    if (strcmp (ROMName, "LOVE QUEST") == 0)
    {
		RomPatch (0x1385ec, 0xd0, 0xea);
		RomPatch (0x1385ed, 0xb2, 0xea);
    }
	//BNE D0 into nops
	
	//seems like the next instruction is a BRA
	//otherwise, this one's too complex for MKendora
    // Nangoku Syonen Papuwa Kun
    if (strcmp (ROMName, "NANGOKUSYONEN PAPUWA") == 0)
		RomPatch (0x1f0d1, 0xa0, 0x6b);
	//turns an LDY into an RTL?
	
	//this is a cmp on $00:2140
    // Super Batter Up
    if (strcmp (ROMName, "Super Batter Up") == 0)
    {
		RomPatch (0x27ae0, 0xd0, 0xea);
		RomPatch (0x27ae1, 0xfa, 0xea);
    }
	//BNE

	//---------------------------------------------------
	// Specific patches for 3DS port.
	//---------------------------------------------------
	// Hack for screen palette handling.
	//
	SNESGameFixes.PaletteCommitLine = -1;
	
	if (strcmp (ROMName, "Secret of MANA") == 0 ||
		strcmp (ROMName, "SeikenDensetsu 2") == 0)
	{
		// Game hack: Dialog palette colours.
		SNESGameFixes.PaletteCommitLine = -2;		// commit palette only at first scan line.
	}
	if (strcmp (ROMName, "Bahamut Lagoon") == 0 ||
		strcmp (ROMName, "Bahamut Lagoon Eng v3") == 0)
	{
		// Game hack: Dialog palette colours.
		SNESGameFixes.PaletteCommitLine = 1;		// commit palette only at first scan line.
	}
	if (strcmp (ROMName, "GUN HAZARD") == 0)
	{
		// Game hack: flashing sky colors
		SNESGameFixes.PaletteCommitLine = 1;		// commit palette only at first scan line.
	}
	if (strncmp (ROMName, "JUDGE DREDD THE MOVIE", 11) == 0)
	{
		SNESGameFixes.PaletteCommitLine = -2;		// do a FLUSH_REDRAW
	}
	if (strcmp (ROMName, "WILD GUNS") == 0)
	{
		SNESGameFixes.PaletteCommitLine = -2;		// do a FLUSH_REDRAW
	}
	if (strcmp (ROMName, "BATMAN FOREVER") == 0)
	{
		SNESGameFixes.PaletteCommitLine = -2;		// do a FLUSH_REDRAW
	}
	if (strcmp (ROMName, "KIRBY SUPER DELUXE") == 0)
	{
		SNESGameFixes.PaletteCommitLine = -2;		// do a FLUSH_REDRAW
	}
	

	// Hack for Final Fantasy Mystic Quest
	// Since it uses SRAM to update values often
	// we delay the saving to 10 seconds.
	//
	// Fix: Included star ocean to save every minute.
	//
	Settings.AutoSaveDelay = 60;
	if (strcmp (ROMName, "FF MYSTIC QUEST") == 0 ||
		strcmp (ROMName, "MYSTIC QUEST LEGEND") == 0)
	{
		Settings.AutoSaveDelay = 600;
	}
	// For star ocean we extend the auto-save interval to 1 minute
	if (strcmp (ROMName, "Star Ocean") == 0)
	{
		Settings.AutoSaveDelay = 3600;
	}
	// For treasure hunter - 1 minute.
	if (strcmp (ROMId, "AEGJ") == 0)
	{
		Settings.AutoSaveDelay = 3600;
	}

	// Hack for Power Rangers Fighting Edition
	//
	SNESGameFixes.IRQCycleCount = 3;
	if (strcmp (ROMName, "POWER RANGERS FIGHT") == 0)
		SNESGameFixes.IRQCycleCount = 0;

	// Speed hacks (experimental)
	// May load from a file in the future
	//
	SNESGameFixes.SpeedHackCount = 0;
	SNESGameFixes.AceONeraeHack = false;
	if (strcmp (ROMName, "YOSHI'S ISLAND") == 0)
	{
		SpeedHackAdd(0x0080F4, 54, 0x30, 0xfb, -1, -1);  // US + EUR version
	}
	if (strcmp (ROMName, "SUPER MARIO KART") == 0)
	{
		SpeedHackAdd(0x80805E, 46, 0xf0, 0xfc, -1, -1);  // US + EUR version
	}
	if (strcmp (ROMName, "F-ZERO") == 0)
	{
		SpeedHackAdd(0x00803C, 46, 0xf0, 0xfc, -1, -1);  // US + EUR version
	}
	if (strcmp (ROMName, "\xb4\xb0\xbd\xa6\xc8\xd7\xb4\x21") == 0)	// Ace o Nerae
	{
		SNESGameFixes.AceONeraeHack = true;
		SpeedHackAdd(0x80C458, -1, 0x10, 0xfb);  
	}
	if (strcmp (ROMName, "AXELAY") == 0)
	{
		SpeedHackAdd(0x00893D, -1, 0xf0, 0xdb, -1, -1);  // US + EUR version
	}

	int instructionSet = 0;

	// SA1 Game's Speed Hack
	//
	if (strcmp (ROMName, "SUPER MARIO RPG") == 0)
	{
		SpeedHackAdd(0xC302FF, -1, 0xF0, 0xFC);  // US version
		SpeedHackAdd(0x7FF7AF, -1, 0xF0, 0xFB);  // 
		SpeedHackAdd(0xC202E9, -1, 0xD0, 0xFB);	 //
		SpeedHackSA1Add(0xC08171, 0xF0, 0xFC);
		instructionSet = 1;
	}
	if (strcmp (ROMName, "KIRBY'S DREAM LAND 3") == 0)
	{
		SpeedHackAdd(0x00949B, -1, 0xF0, 0xFB);  
		SpeedHackSA1Add(0x0082D7, 0xF0, 0xFB);
		//SpeedHackSA1Add(0x00A970, 0xF0, 0xFB);
		instructionSet = 1;
	}
	if (strcmp (ROMName, "OSHABERI PARODIUS") == 0)
	{
		SpeedHackAdd(0x80814A, -1, 0x80, 0xF0);  
		SpeedHackSA1Add(0x8084E8, 0x80, 0xFB);
		instructionSet = 1;
	}
    /* KIRBY SUPER DELUXE JAP */
    if (strcmp (ROMId, "AKFJ") == 0)
    {
		SpeedHackAdd(0x008A59, -1, 0x80, 0xF0);  
		SpeedHackSA1Add(0x008C96, 0x30, 0x09);
		instructionSet = 1;
    }
    /* KIRBY SUPER DELUXE US */
    if (strcmp (ROMId, "AKFE") == 0)
    {
		SpeedHackAdd(0x008A59, -1, 0xF0, 0xFB);  
		SpeedHackSA1Add(0x008CBB, 0x30, 0x09);
		instructionSet = 1;
    }
	/* MARVELOUS */
    if (strcmp (ROMId, "AVRJ") == 0)
    {
		SpeedHackAdd(0x009941, -1, 0xF0, 0xFB);  
		SpeedHackSA1Add(0x0085F4, 0xf0, 0xfc);
		instructionSet = 1;
    }
	/* SUPER ROBOT TAISEN - MASOUKISHIN */
    if (strcmp (ROMId, "ALXJ") == 0)
    {
		SpeedHackAdd(0x00F0AF, -1, 0x70, 0xFC);
		SpeedHackSA1Add(0x00EC9F, 0xf0, 0xfb);
		SA1.WaitByteAddress1 = FillRAM + 0x003072;
		instructionSet = 1;
    }
    /* PANIC BOMBER WORLD */
    if (strcmp (ROMId, "APBJ") == 0)
    {
		SpeedHackAdd(0x0082AA, -1, 0xF0, 0xFC);
		SpeedHackSA1Add(0x00857A, 0xCB);
		SA1.WaitAddress = SA1.Map [0x00857a >> MEMMAP_SHIFT] + 0x857a;
    }
    /* Dragon Ballz HD */
    if (strcmp (ROMId, "AZIJ") == 0)
    {
		SpeedHackAdd(0x008031, -1, 0xD0, 0xFB); 
		SpeedHackSA1Add(0x0080BF, 0x4C, 0x83, 0x80);
		instructionSet = 1;
    }
    /* SFC SDGUNDAMGNEXT */
    if (strcmp (ROMId, "ZX3J") == 0)
    {
		SpeedHackSA1Add(0x01AE76, 0xD0, 0xFC);
		instructionSet = 1;
    }
    /* POWER RANGERS 4 */
    if (strcmp (ROMId, "A4RE") == 0)
    {
		SpeedHackAdd(0x0082B0, -1, 0xF0, 0xFC);
		SpeedHackSA1Add(0x00989F, 0x80, 0xF8);
		instructionSet = 1;
    }
    /* DAISENRYAKU EXPERTWW2 */
    if (strcmp (ROMId, "AEVJ") == 0)
    {
		SpeedHackSA1Add(0x0ED18F, 0xF0, 0xFC);
		instructionSet = 1;
    }
    /* AUGUSTA3 MASTERS NEW */
    if (strcmp (ROMId, "AO3J") == 0)
    {
		SpeedHackAdd(0x00CFAF, -1, 0xF0, 0xFA);
		SpeedHackSA1Add(0x00DDDE, 0xF0, 0xFB);
		instructionSet = 1;
    }
    /* Bass Fishing */
    if (strcmp (ROMId, "ZBPJ") == 0)
    {
		SpeedHackSA1Add(0x0093F4, 0xF0, 0xFB);
		instructionSet = 1;
    }
    /* J96 DREAM STADIUM */
    if (strcmp (ROMId, "AJ6J") == 0)
    {
		SpeedHackSA1Add(0xC0F74A, 0x80, 0xFE);
    }
    /* JumpinDerby */
    if (strcmp (ROMId, "AJUJ") == 0)
    {
		SpeedHackSA1Add(0x00d926, 0x80, 0xFE);
    }
    /* SHINING SCORPION */
    if (strcmp (ROMId, "A4WJ") == 0)
    {
		//SpeedHackAdd(0xC00185, -1, 0xF0, 0xFC);
		SpeedHackSA1Add(0xC048C0, 0x80, 0xFC);
    }
    /* PEBBLE BEACH NEW */
    if (strcmp (ROMId, "AONJ") == 0)
    {
		SpeedHackSA1Add(0x00DF36, 0xF0, 0xFB);
		instructionSet = 1;
    }
    /* PGA EUROPEAN TOUR */
    if (strcmp (ROMId, "AEPE") == 0)
    {
		SpeedHackSA1Add(0x003704, 0xF0, 0xFA);
		instructionSet = 1;
    }
    /* PGA TOUR 96 */
    if (strcmp (ROMId, "A3GE") == 0)
    {
		SpeedHackSA1Add(0x003704, 0xF0, 0xFA);
		instructionSet = 1;
    }
    /* SD F1 GRAND PRIX */
    if (strcmp (ROMId, "AGFJ") == 0)
    {
		SpeedHackSA1Add(0x0181BC, 0x80, 0xFE);
    }
	
	ApplySpeedHackPatches();

	// Use the
	//   0 - Default faster instruction set,
	//   1 - The one that allows waking the SA1 from non-executing state.
	//
	S9xUseInstructionSet(instructionSet);
	
}

// Read variable size MSB int from a file
static long ReadInt (FILE *f, unsigned nbytes)
{
    long v = 0;
    while (nbytes--)
    {
		int c = fgetc(f);
		if (c == EOF) 
			return -1;
		v = (v << 8) | (c & 0xFF);
    }
    return (v);
}

#define IPS_EOF 0x00454F46l

void CMemory::CheckForIPSPatch (const char *rom_filename, bool8 header,
								int32 &rom_size)
{
    char  dir [_MAX_DIR + 1];
    char  drive [_MAX_DRIVE + 1];
    char  name [_MAX_FNAME + 1];
    char  ext [_MAX_EXT + 1];
    char  fname [_MAX_PATH + 1];
    FILE  *patch_file  = NULL;
    long  offset = header ? 512 : 0;
	
    _splitpath (rom_filename, drive, dir, name, ext);
    _makepath (fname, drive, dir, name, "ips");
    
    if (!(patch_file = fopen (fname, "rb")))
    {
		if (!(patch_file = fopen (S9xGetFilename (".ips"), "rb")))
			return;
    }
	
    if (fread (fname, 1, 5, patch_file) != 5 ||
		strncmp (fname, "PATCH", 5) != 0)
    {
		fclose (patch_file);
		return;
    }
	
	printf ("Patching IPS. It may take a while...\n");
    int32 ofs;
	
    for (;;)
    {
		long len;
		long rlen;
		int  rchar;
		
		ofs = ReadInt (patch_file, 3);
		if (ofs == -1)
			goto err_eof;
		
		if (ofs == IPS_EOF) 
			break;
		
		ofs -= offset;
		
        len = ReadInt (patch_file, 2);
		if (len == -1)
			goto err_eof;
		
		/* Apply patch block */
		if (len)
		{
			if (ofs + len > MAX_ROM_SIZE)
				goto err_eof;
			
			while (len--)
			{
				rchar = fgetc (patch_file);
				if (rchar == EOF) 
					goto err_eof;
				ROM [ofs++] = (uint8) rchar;
            }
			if (ofs > rom_size)
				rom_size = ofs;
		}
		else
		{
			rlen = ReadInt (patch_file, 2);
			if (rlen == -1) 
				goto err_eof;
			
			rchar = fgetc (patch_file);
			if (rchar == EOF) 
				goto err_eof;
			
			if (ofs + rlen > MAX_ROM_SIZE)
				goto err_eof;
			
			while (rlen--) 
				ROM [ofs++] = (uint8) rchar;
			
			if (ofs > rom_size)
				rom_size = ofs;
		}
    }
	
    // Check if ROM image needs to be truncated
    ofs = ReadInt (patch_file, 3);
    if (ofs != -1 && ofs - offset < rom_size)
    {
		// Need to truncate ROM image
		rom_size = ofs - offset;
    }
    fclose (patch_file);
    return;
	
err_eof:
    if (patch_file) 
		fclose (patch_file);
}

void CMemory::ParseSNESHeader(uint8* RomHeader)
{
	bool8	bs = Settings.BS & !Settings.BSXItself;

	strncpy(ROMName, (char *) &RomHeader[0x10], ROM_NAME_LEN - 1);
	if (bs)
		memset(ROMName + 16, 0x20, ROM_NAME_LEN - 17);

	if (bs)
	{
		if (!(((RomHeader[0x29] & 0x20) && CalculatedSize <  0x100000) ||
			 (!(RomHeader[0x29] & 0x20) && CalculatedSize == 0x100000)))
			printf("BS: Size mismatch\n");

		// FIXME
		int	p = 0;
		while ((1 << p) < (int) CalculatedSize)
			p++;
		ROMSize = p - 10;
	}
	else
		ROMSize = RomHeader[0x27];

	SRAMSize  = bs ? 5 /* BS-X */    : RomHeader[0x28];
	ROMSpeed  = bs ? RomHeader[0x28] : RomHeader[0x25];
	ROMType   = bs ? 0xE5 /* BS-X */ : RomHeader[0x26];
	ROMRegion = bs ? 0               : RomHeader[0x29];

	ROMChecksum           = RomHeader[0x2E] + (RomHeader[0x2F] << 8);
	ROMComplementChecksum = RomHeader[0x2C] + (RomHeader[0x2D] << 8);

	memmove(ROMId, &RomHeader[0x02], 4);

	if(RomHeader[0x2A]==0x33)
		memmove (CompanyId, &RomHeader [0], 2);
	else 
		sprintf(CompanyId, "%02X", RomHeader[0x2A]);
}

#undef INLINE
#define INLINE
#include "getset.h"

