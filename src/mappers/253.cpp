//Thanks to VirtualNES developer for reverse engineering this mapper 
//first screen add 32 to chr
//second screen at 0x42000
#include "mapinc.h"

static uint8 reg[9], VRAM_switch;
static int32 irq_enable, irq_counter, irq_latch, irq_clock; 

static void Mapper253_IRQHook(int cycles)
{
    if( irq_enable & 0x02 ) 
    {
        if( (irq_clock+=cycles) >= 0x72 ) 
        {
            irq_clock -= 0x72;
            if( irq_counter == 0xFF ) 
            {
                irq_counter = irq_latch;
                irq_enable = (irq_enable & 0x01) * 3;
                X6502_IRQBegin(FCEU_IQEXT);
            } else 
            {
                irq_counter++;
            }
        }
    }
}

static void	SetBank_PPUSUB(int bank, int page)
{
    if(page == 0x88)
    {
        VRAM_switch = 0;
        return;
    }
    else if(page == 0xC8)
    {
        VRAM_switch = 1;
        return;
    }
    if ((page == 4) || (page == 5)) 
    {
        printf("Page in 4 5: %d\n", page);
        if(VRAM_switch == 0)
            setchr1( bank << 10, page ); //CHR-ROM
        else 
            setchr1( bank << 10, page ); //CHR-RAM
    } 
    else 
	{
		printf("Page: %d\n", page);
        setchr1(bank << 10, page);
	}
}

static DECLFW(Mapper253_Write)
{
    printf("Address: 0x%X V: 0x%X\n", A, V);
    if (A == 0x8010) //8kb select at 0x8000
    {
        setprg8(0x8000, V);
        return;
    }

    if (A == 0xA010) //8kb select at 0xA000
    {
        setprg8(0xA000, V);
        return;
    }

    if (A == 0x9400) //Mirroring
    {
        V &= 0x03;
        switch (V)
        {
            case 0: 
                setmirror(MI_V);
                break;
            case 1:
                setmirror(MI_H);
                break;
            case 2:
                setmirror(MI_0);
                break;
            case 3:
                setmirror(MI_1);
                break;
        }
    }

    switch (A & 0xF00C)
    {
        case 0xB000:
            reg[0] = (reg[0] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 0, reg[0] );
            break;	
        case 0xB004:
            reg[0] = (reg[0] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 0, reg[0] + ((V>>4)*0x100) );
            break;
        case 0xB008:
            reg[1] = (reg[1] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 1, reg[1] );
            break;
        case 0xB00C:
            reg[1] = (reg[1] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 1, reg[1] + ((V>>4)*0x100) );
            break;
        case 0xC000:
            reg[2] = (reg[2] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 2, reg[2] );
            break;
        case 0xC004:
            reg[2] = (reg[2] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 2, reg[2] + ((V>>4)*0x100) );
            break;
        case 0xC008:
            reg[3] = (reg[3] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 3, reg[3] );
            break;
        case 0xC00C:
            reg[3] = (reg[3] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 3, reg[3] + ((V>>4)*0x100) );
            break;
        case 0xD000:
            reg[4] = (reg[4] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 4, reg[4] );
            break;
        case 0xD004:
            reg[4] = (reg[4] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 4, reg[4] + ((V>>4)*0x100) );
            break;
        case 0xD008:
            reg[5] = (reg[5] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 5, reg[5] );
            break;
        case 0xD00C:
            reg[5] = (reg[5] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 5, reg[5] + ((V>>4)*0x100) );
            break;
        case 0xE000:
            reg[6] = (reg[6] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 6, reg[6] );
            break;
        case 0xE004:
            reg[6] = (reg[6] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 6, reg[6] + ((V>>4)*0x100) );
            break;
        case 0xE008:
            reg[7] = (reg[7] & 0xF0) | (V & 0x0F);
            SetBank_PPUSUB( 7, reg[7] );
            break;
        case 0xE00C:
            reg[7] = (reg[7] & 0x0F) | ((V & 0x0F) << 4);
            SetBank_PPUSUB( 7, reg[7] + ((V>>4)*0x100) );
            break;
        case 0xF000:
            irq_latch = (irq_latch & 0xF0) | (V & 0x0F);
            break;
        case 0xF004:
            irq_latch = (irq_latch & 0x0F) | ((V & 0x0F) << 4);
            break;
        case 0xF008:
            irq_enable = V & 0x03;
            if (irq_enable & 0x02) 
            {
                irq_counter = irq_latch;
                irq_clock = 0;
            }
            X6502_IRQEnd(FCEU_IQEXT);
            break;
        default:
			break;
            //printf("Not handled 0x%X 0x%X", A, V);
    }
}

static void Mapper253_Power(void)
{
    int i;
    for (i = 0; i != 8; ++i)
        reg[i] = i;

    reg[8] = 0;
    irq_enable  = 0; 
    irq_counter = 0;
    irq_latch   = 0;
    irq_clock   = 0;
    VRAM_switch = 0;
   
    setprg16(0x8000, 0); //first bank
    setprg16(0xC000, head.ROM_size - 1); //last bank
    setchr8(0);

    SetWriteHandler(0x8000, 0xFFFF, Mapper253_Write);
    SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void Mapper253_Close(void)
{
}

void Mapper253_Init(CartInfo *info)
{
    info->Power = Mapper253_Power;
    MapIRQHook = Mapper253_IRQHook;
    info->Close = Mapper253_Close;
    SetWriteHandler(0x8000, 0xFFFF, Mapper253_Write);
    SetReadHandler(0x8000, 0xFFFF, CartBR);
}

