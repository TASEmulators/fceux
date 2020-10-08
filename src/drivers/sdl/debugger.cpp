#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef _GTK3
#include <gdk/gdkkeysyms-compat.h>
#endif

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../version.h"
#include "../../video.h"
#include "../../movie.h"
#include "../../palette.h"
#include "../../fds.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../../asm.h"
#include "../../ppu.h"
#include "../../x6502.h"
#include "../common/configSys.h"

#include "sdl.h"
#include "gui.h"
#include "dface.h"
#include "input.h"
#include "config.h"
#include "debugger.h"

extern Config *g_config;
extern int vblankScanLines;
extern int vblankPixel;

static int  breakpoint_hit = 0;
static void updateAllDebugWindows(void);
//*******************************************************************************************************
// Debugger Window
//*******************************************************************************************************
//

struct dbg_asm_entry_t
{
	int  addr;
	int  bank;
	int  rom;
	int  size;
	int  line;
	uint8  opcode[3];
	std::string  text;


	dbg_asm_entry_t(void)
	{
		addr = 0; bank = 0; rom = -1; 
		size = 0; line = 0;

		for (int i=0; i<3; i++)
		{
			opcode[i] = 0;
		}
	}
};

struct debuggerWin_t
{
	GtkWidget *win;
	GtkTextView *textview;
	GtkTextBuffer *textbuf;
	GtkTextView *stackview;
	GtkTextBuffer *stackTextBuf;
	GtkTreeStore *bp_store;
	GtkTreeStore *bkm_store;
	GtkWidget *bp_tree;
	GtkWidget *bkm_tree;
	GtkWidget *pc_entry;
	GtkWidget *A_entry;
	GtkWidget *X_entry;
	GtkWidget *Y_entry;
	GtkWidget *P_N_chkbox;
	GtkWidget *P_V_chkbox;
	GtkWidget *P_U_chkbox;
	GtkWidget *P_B_chkbox;
	GtkWidget *P_D_chkbox;
	GtkWidget *P_I_chkbox;
	GtkWidget *P_Z_chkbox;
	GtkWidget *P_C_chkbox;
	GtkWidget *add_bp_button;
	GtkWidget *edit_bp_button;
	GtkWidget *del_bp_button;
	GtkWidget *stack_frame;
	GtkWidget *ppu_label;
	GtkWidget *sprite_label;
	GtkWidget *scanline_label;
	GtkWidget *pixel_label;
	GtkWidget *cpu_label1;
	GtkWidget *cpu_label2;
	GtkWidget *instr_label1;
	GtkWidget *instr_label2;
	GtkWidget *bp_start_entry;
	GtkWidget *bp_end_entry;
	GtkWidget *bp_read_chkbox;
	GtkWidget *bp_write_chkbox;
	GtkWidget *bp_execute_chkbox;
	GtkWidget *bp_enable_chkbox;
	GtkWidget *bp_forbid_chkbox;
	GtkWidget *bp_cond_entry;
	GtkWidget *bp_name_entry;
	GtkWidget *cpu_radio_btn;
	GtkWidget *ppu_radio_btn;
	GtkWidget *sprite_radio_btn;
	GtkWidget *badop_chkbox;
	GtkWidget *brkcycles_chkbox;
	GtkWidget *brkinstrs_chkbox;
	GtkWidget *brk_cycles_lim_entry;
	GtkWidget *brk_instrs_lim_entry;
	GtkWidget *seektoEntry;

	int  dialog_op;
	int  bpEditIdx;
	int  ctx_menu_addr;
	char displayROMoffsets;

	dbg_asm_entry_t  *asmPC;
	std::vector <dbg_asm_entry_t*> asmEntry;

	debuggerWin_t(void)
	{
		win = NULL;
		textview = NULL;
		textbuf = NULL;
		bp_store = NULL;
		bp_tree = NULL;
		bkm_store = NULL;
		bkm_tree = NULL;
		pc_entry = NULL;
		A_entry = NULL;
		X_entry = NULL;
		Y_entry = NULL;
		stackview = NULL;
		stackTextBuf = NULL;
		ppu_label = NULL;
		sprite_label = NULL;
		scanline_label = NULL;
		pixel_label = NULL;
		cpu_label1 = NULL;
		cpu_label2 = NULL;
		instr_label1 = NULL;
		instr_label2 = NULL;
		bp_start_entry = NULL;
		bp_end_entry = NULL;
		bp_cond_entry = NULL;
		bp_name_entry = NULL;
		bp_read_chkbox = NULL;
		bp_write_chkbox = NULL;
		bp_execute_chkbox = NULL;
		bp_enable_chkbox = NULL;
		bp_forbid_chkbox = NULL;
		add_bp_button = NULL;
		edit_bp_button = NULL;
		del_bp_button = NULL;
	   cpu_radio_btn = NULL;
	   ppu_radio_btn = NULL;
	   sprite_radio_btn = NULL;
		stack_frame = NULL;
		P_N_chkbox = NULL;
		P_V_chkbox = NULL;
		P_U_chkbox = NULL;
		P_B_chkbox = NULL;
		P_D_chkbox = NULL;
		P_I_chkbox = NULL;
		P_Z_chkbox = NULL;
		P_C_chkbox = NULL;
		badop_chkbox = NULL;
		brkcycles_chkbox = NULL;
		brkinstrs_chkbox = NULL;
		brk_cycles_lim_entry = NULL;
		brk_instrs_lim_entry = NULL;
		seektoEntry = NULL;
		dialog_op = 0;
		bpEditIdx = -1;
		ctx_menu_addr = 0;
		displayROMoffsets = 0;
		asmPC = NULL;
	}
	
	~debuggerWin_t(void)
	{
		asmClear();
	}

	void  asmClear(void);
	void  bpListUpdate(void);
	void  updateViewPort(void);
	void  updateRegisterView(void);
	void  updateAssemblyView(void);
	int   get_bpList_selrow(void);
	int   getAsmLineFromAddr(int addr);
	int   scrollAsmLine(int line);
	int   seekAsmPC(void);
	int   seekAsmAddr(int addr);
	void  setRegsFromEntry(void);
};

static std::list <debuggerWin_t*> debuggerWinList;

void debuggerWin_t::setRegsFromEntry(void)
{
	X.PC = strtol( gtk_entry_get_text( GTK_ENTRY( pc_entry ) ), NULL, 16 );
	X.A  = strtol( gtk_entry_get_text( GTK_ENTRY(  A_entry ) ), NULL, 16 );
	X.X  = strtol( gtk_entry_get_text( GTK_ENTRY(  X_entry ) ), NULL, 16 );
	X.Y  = strtol( gtk_entry_get_text( GTK_ENTRY(  Y_entry ) ), NULL, 16 );
}

void debuggerWin_t::asmClear(void)
{
	for (size_t i=0; i<asmEntry.size(); i++)
	{
		delete asmEntry[i];
	}
	asmEntry.clear();
}

int  debuggerWin_t::getAsmLineFromAddr(int addr)
{
	int  line = -1;
	int  incr, nextLine;
	int  run = 1;

	if ( asmEntry.size() <= 0 )
	{
      return -1;
	}
	incr = asmEntry.size() / 2;

	if ( addr < asmEntry[0]->addr )
	{
		return 0;
	}
	else if ( addr > asmEntry[ asmEntry.size() - 1 ]->addr )
	{
		return asmEntry.size() - 1;
	}

	if ( incr < 1 ) incr = 1;

	nextLine = line = incr;

	// algorithm to efficiently find line from address. Starts in middle of list and 
	// keeps dividing the list in 2 until it arrives at an answer.
	while ( run )
	{
		//printf("incr:%i   line:%i  addr:%04X   delta:%i\n", incr, line, asmEntry[line]->addr, addr - asmEntry[line]->addr);

		if ( incr == 1 )
		{
			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + 1;
				if ( asmEntry[line]->addr > nextLine )
				{
					break;
				}
				line = nextLine;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - 1;
				if ( asmEntry[line]->addr < nextLine )
				{
					break;
				}
				line = nextLine;
			}
			else 
			{
				run = 0; break;
			}
		} 
		else
		{
			incr = incr / 2; 
			if ( incr < 1 ) incr = 1;

			if ( asmEntry[line]->addr < addr )
			{
				nextLine = line + incr;
			}
			else if ( asmEntry[line]->addr > addr )
			{
				nextLine = line - incr;
			}
			else
			{
				run = 0; break;
			}
			line = nextLine;
		}
	}

	//for (size_t i=0; i<asmEntry.size(); i++)
	//{
	//	if ( asmEntry[i]->addr >= addr )
	//	{
   //      line = i; break;
	//	}
	//}

	return line;
}

int  debuggerWin_t::scrollAsmLine(int line)
{
	GtkTextIter iter;

	if ( line < 0 )
	{
		return -1;
	}
	gtk_text_buffer_get_iter_at_line( textbuf, &iter, line );
	gtk_text_view_scroll_to_iter ( textview, &iter, 0.0, 1, 0.0, 0.50 );
	gtk_text_buffer_place_cursor( textbuf, &iter );

   return 0;
}

int  debuggerWin_t::seekAsmPC(void)
{
	int line;

	if ( asmPC == NULL )
	{
		line = getAsmLineFromAddr( X.PC );
	}
	else
	{
		line = asmPC->line;
	}

	scrollAsmLine( line );

	return 0;
}

int  debuggerWin_t::seekAsmAddr( int addr )
{
	int line;

	line = getAsmLineFromAddr( addr );

	scrollAsmLine( line );

	return 0;
}

int  debuggerWin_t::get_bpList_selrow(void)
{
	int retval = -1, numListRows;
	GList *selListRows, *tmpList;
	GtkTreeModel *model = NULL;
	GtkTreeSelection *treeSel;

	treeSel =
		gtk_tree_view_get_selection (GTK_TREE_VIEW(bp_tree) );

	numListRows = gtk_tree_selection_count_selected_rows (treeSel);

	if (numListRows == 0)
	{
		return retval;
	}
	//printf("Number of Rows Selected: %i\n", numListRows );

	selListRows = gtk_tree_selection_get_selected_rows (treeSel, &model);

	tmpList = selListRows;

	while (tmpList)
	{
		int depth;
		int *indexArray;
		GtkTreePath *path = (GtkTreePath *) tmpList->data;

		depth = gtk_tree_path_get_depth (path);
		indexArray = gtk_tree_path_get_indices (path);

		if (depth > 0)
		{
			retval = indexArray[0];
			break;
		}
		tmpList = tmpList->next;
	}

	g_list_free_full (selListRows, (GDestroyNotify) gtk_tree_path_free);

	return retval;
}

void debuggerWin_t::bpListUpdate(void)
{
	GtkTreeIter iter;
	char line[256], addrStr[32], flags[16], enable;

	gtk_tree_store_clear( bp_store );

	for (int i=0; i<numWPs; i++)
	{
		gtk_tree_store_append (bp_store, &iter, NULL);	// aquire iter

		if ( watchpoint[i].endaddress > 0 )
		{
			sprintf( addrStr, "$%04X-%04X:", watchpoint[i].address, watchpoint[i].endaddress );
		}
		else
		{
			sprintf( addrStr, "$%04X:", watchpoint[i].address );
		}

		flags[0] = (watchpoint[i].flags & WP_E) ? 'E' : '-';

		if ( watchpoint[i].flags & BT_P )
		{
			flags[1] = 'P';
		}
		else if ( watchpoint[i].flags & BT_S )
		{
			flags[1] = 'S';
		}
		else
		{
			flags[1] = 'C';
		}

		flags[2] = (watchpoint[i].flags & WP_R) ? 'R' : '-';
		flags[3] = (watchpoint[i].flags & WP_W) ? 'W' : '-';
		flags[4] = (watchpoint[i].flags & WP_X) ? 'X' : '-';
		flags[5] = (watchpoint[i].flags & WP_F) ? 'F' : '-';
		flags[6] = 0;

		enable = (watchpoint[i].flags & WP_E) ? 1 : 0;

		strcpy( line, addrStr );
		strcat( line, flags   );

		if (watchpoint[i].desc )
		{
			strcat( line, " ");
			strcat( line, watchpoint[i].desc);
			strcat( line, " ");
		}

		if (watchpoint[i].condText )
		{
			strcat( line, " Condition:");
			strcat( line, watchpoint[i].condText);
			strcat( line, " ");
		}
		gtk_tree_store_set( bp_store, &iter, 0, enable, 1, line, -1 );
	}

}

void  debuggerWin_t::updateRegisterView(void)
{
	int stackPtr;
	char stmp[64];
	char str[32], str2[32];
	std::string  stackLine;

	sprintf( stmp, "%04X", X.PC );

	gtk_entry_set_text( GTK_ENTRY(pc_entry), stmp );

	sprintf( stmp, "%02X", X.A );

	gtk_entry_set_text( GTK_ENTRY(A_entry), stmp );

	sprintf( stmp, "%02X", X.X );

	gtk_entry_set_text( GTK_ENTRY(X_entry), stmp );

	sprintf( stmp, "%02X", X.Y );

	gtk_entry_set_text( GTK_ENTRY(Y_entry), stmp );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_N_chkbox ), (X.P & N_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_V_chkbox ), (X.P & V_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_U_chkbox ), (X.P & U_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_B_chkbox ), (X.P & B_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_D_chkbox ), (X.P & D_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_I_chkbox ), (X.P & I_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_Z_chkbox ), (X.P & Z_FLAG) ? TRUE : FALSE );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( P_C_chkbox ), (X.P & C_FLAG) ? TRUE : FALSE );

	stackPtr = X.S | 0x0100;

	sprintf( stmp, "Stack: %04X", stackPtr );
	gtk_frame_set_label( GTK_FRAME(stack_frame), stmp );

	stackPtr++;

	if ( stackPtr <= 0x01FF )
	{
		sprintf( stmp, "%02X", GetMem(stackPtr) );

		stackLine.assign( stmp );

		for (int i = 1; i < 128; i++)
		{
			//tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
			stackPtr++;
			if (stackPtr > 0x1FF)
				break;
			if ((i & 7) == 0)
				sprintf( stmp, ",\r\n%02X", GetMem(stackPtr) );
			else
				sprintf( stmp, ",%02X", GetMem(stackPtr) );

			stackLine.append( stmp );
		}
	}

	gtk_text_buffer_set_text( stackTextBuf, stackLine.c_str(), -1 ) ;

	// update counters
	int64 counter_value = timestampbase + (uint64)timestamp - total_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf( stmp, "CPU Cycles: %llu", counter_value);

	gtk_label_set_text( GTK_LABEL(cpu_label1), stmp );

	//SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_CYCLES_COUNT, str);
	counter_value = timestampbase + (uint64)timestamp - delta_cycles_base;
	if (counter_value < 0)	// sanity check
	{
		ResetDebugStatisticsCounters();
		counter_value = 0;
	}
	sprintf(stmp, "(+%llu)", counter_value);

	gtk_label_set_text( GTK_LABEL(cpu_label2), stmp );

	sprintf(stmp, "Instructions: %llu", total_instructions);
	gtk_label_set_text( GTK_LABEL(instr_label1), stmp );

	sprintf(stmp, "(+%llu)", delta_instructions);
	gtk_label_set_text( GTK_LABEL(instr_label2), stmp );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(badop_chkbox    ), FCEUI_Debugger().badopbreak );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(brkcycles_chkbox), break_on_cycles             );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(brkinstrs_chkbox), break_on_instructions       );

	sprintf(stmp, "%llu", break_cycles_limit);
	gtk_entry_set_text( GTK_ENTRY(brk_cycles_lim_entry), stmp );

	sprintf(stmp, "%llu", break_instructions_limit);
	gtk_entry_set_text( GTK_ENTRY(brk_instrs_lim_entry), stmp );

	sprintf(stmp, "PPU: 0x%04X", (int)FCEUPPU_PeekAddress());
	gtk_label_set_text( GTK_LABEL(ppu_label), stmp );

	sprintf(stmp, "Sprite: 0x%02X", PPU[3] );
	gtk_label_set_text( GTK_LABEL(sprite_label), stmp );

	extern int linestartts;
	#define GETLASTPIXEL    (PAL?((timestamp*48-linestartts)/15) : ((timestamp*48-linestartts)/16) )
	
	int ppupixel = GETLASTPIXEL;

	if (ppupixel>341)	//maximum number of pixels per scanline
		ppupixel = 0;	//Currently pixel display is borked until Run 128 lines is clicked, this keeps garbage from displaying

	// If not in the 0-239 pixel range, make special cases for display
	if (scanline == 240 && vblankScanLines < (PAL?72:22))
	{
		if (!vblankScanLines)
		{
			// Idle scanline (240)
			sprintf(str, "%d", scanline);	// was "Idle %d"
		} else if (scanline + vblankScanLines == (PAL?311:261))
		{
			// Pre-render
			sprintf(str, "-1");	// was "Prerender -1"
		} else
		{
			// Vblank lines (241-260/310)
			sprintf(str, "%d", scanline + vblankScanLines);	// was "Vblank %d"
		}
		sprintf(str2, "%d", vblankPixel);
	} else
	{
		// Scanlines 0 - 239
		sprintf(str, "%d", scanline);
		sprintf(str2, "%d", ppupixel);
	}

	if(newppu)
	{
		sprintf(str ,"%d",newppu_get_scanline());
		sprintf(str2,"%d",newppu_get_dot());
	}

	sprintf( stmp, "Scanline: %s", str );
	gtk_label_set_text( GTK_LABEL(scanline_label), stmp );

	sprintf( stmp, "Pixel: %s", str2 );
	gtk_label_set_text( GTK_LABEL(pixel_label), stmp );

}

// This function is for "smart" scrolling...
// it attempts to scroll up one line by a whole instruction
static int InstructionUp(int from)
{
	int i = std::min(16, from), j;

	while (i > 0)
	{
		j = i;
		while (j > 0)
		{
			if (GetMem(from - j) == 0x00)
				break;	// BRK usually signifies data
			if (opsize[GetMem(from - j)] == 0)
				break;	// invalid instruction!
			if (opsize[GetMem(from - j)] > j)
				break;	// instruction is too long!
			if (opsize[GetMem(from - j)] == j)
				return (from - j);	// instruction is just right! :D
			j -= opsize[GetMem(from - j)];
		}
		i--;
	}

	// if we get here, no suitable instruction was found
	if ((from >= 2) && (GetMem(from - 2) == 0x00))
		return (from - 2);	// if a BRK instruction is possible, use that
	if (from)
		return (from - 1);	// else, scroll up one byte
	return 0;	// of course, if we can't scroll up, just return 0!
}
//static int InstructionDown(int from)
//{
//	int tmp = opsize[GetMem(from)];
//	if ((tmp))
//		return from + tmp;
//	else
//		return from + 1;		// this is data or undefined instruction
//}

void  debuggerWin_t::updateAssemblyView(void)
{
	int starting_address, start_address_lp, addr, size;
	int instruction_addr, textview_lines_allocated;
	std::string line;
	char chr[64];
	uint8 opcode[3];
	const char *disassemblyText = NULL;
	dbg_asm_entry_t *a;
	GtkTextIter iter, next_iter;
	char pc_found = 0;

	start_address_lp = starting_address = X.PC;

	for (int i=0; i < 0xFFFF; i++)
	{
		//printf("%i: Start Address: 0x%04X \n", i, start_address_lp );

		starting_address = InstructionUp( start_address_lp );

		if ( starting_address == start_address_lp )
		{
			break;
		}
		if ( starting_address < 0x8000 )
		{
			starting_address = start_address_lp;
			break;
		}	
		start_address_lp = starting_address;
	}

	asmClear();

	addr  = starting_address;
	asmPC = NULL;

	gtk_text_buffer_get_start_iter( textbuf, &iter );

	textview_lines_allocated = gtk_text_buffer_get_line_count( textbuf ) - 1;

	//printf("Num Lines: %i\n", textview_lines_allocated );

	for (int i=0; i < 0xFFFF; i++)
	{
		line.clear();

		// PC pointer
		if (addr > 0xFFFF) break;

		a = new dbg_asm_entry_t;

		instruction_addr = addr;

		if ( !pc_found )
		{
			if (addr > X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			}
			else if (addr == X.PC)
			{
				asmPC = a;
				line.assign(">");
				pc_found = 1;
			} 
			else
			{
				line.assign(" ");
			}
		}
		else 
		{
			line.assign(" ");
		}
		a->addr = addr;

		if (addr >= 0x8000)
		{
			a->bank = getBank(addr);
			a->rom  = GetNesFileAddress(addr);

			if (displayROMoffsets && (a->rom != -1) )
			{
				sprintf(chr, " %06X: ", a->rom);
			} 
			else
			{
				sprintf(chr, "%02X:%04X: ", a->bank, addr);
			}
		} 
		else
		{
			sprintf(chr, "  :%04X: ", addr);
		}
		line.append(chr);

		size = opsize[GetMem(addr)];
		if (size == 0)
		{
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			line.append(chr);
		} else
		{
			if ((addr + size) > 0xFFFF)
			{
				while (addr < 0xFFFF)
				{
					sprintf(chr, "%02X        OVERFLOW\n", GetMem(addr++));
					line.append(chr);
				}
				break;
			}
			for (int j = 0; j < size; j++)
			{
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				line.append(chr);
			}
			while (size < 3)
			{
				line.append("   ");  //pad output to align ASM
				size++;
			}

			disassemblyText = Disassemble(addr, opcode);

			if ( disassemblyText )
			{
				line.append( disassemblyText );
			}
		}
		for (int j=0; j<size; j++)
		{
			a->opcode[j] = opcode[j];
		}
		a->size = size;

		// special case: an RTS opcode
		if (GetMem(instruction_addr) == 0x60)
		{
			line.append("-------------------------");
		}

		a->text.assign( line );

		a->line = asmEntry.size();

		if ( i < textview_lines_allocated )
		{
			char *txt;

			//gtk_text_buffer_get_iter_at_line( textbuf, &iter, i );

			next_iter = iter;

			gtk_text_iter_forward_to_line_end( &next_iter );

			//gtk_text_iter_backward_chars( &next_iter, -1 );
			txt = gtk_text_buffer_get_text( textbuf, &iter, &next_iter, FALSE);

			if ( strcmp( txt, line.c_str() ) != 0 )
			{
				//printf("Text is the diff\n%s\n", txt);

				gtk_text_buffer_delete( textbuf, &iter, &next_iter );

				gtk_text_buffer_insert( textbuf, &iter, line.c_str(), -1 );
			}
			else 
			{
				iter = next_iter;
			}
			gtk_text_iter_forward_chars( &iter, 1 );

			g_free(txt);
		}
		else
		{
			line.append("\n");

			gtk_text_buffer_insert( textbuf, &iter, line.c_str(), -1 );
			//gtk_text_buffer_get_end_iter( textbuf, &iter );
		}

		asmEntry.push_back(a);
	}

}

void  debuggerWin_t::updateViewPort(void)
{
	updateRegisterView();
	updateAssemblyView();
	bpListUpdate();
}

static void handleDialogResponse (GtkWidget * w, gint response_id, debuggerWin_t * dw)
{
	//printf("Response %i\n", response_id ); 

	if ( response_id == GTK_RESPONSE_OK )
	{
		const char *txt;

		////printf("Reponse OK\n");
		switch ( dw->dialog_op )
		{
			case 1: // Breakpoint Add
			case 2: // Breakpoint Edit
			{
				int  start_addr = -1, end_addr = -1, type = 0, enable = 1, slot;
				const char *name; 
				const char *cond;

				slot = (dw->dialog_op == 1) ? numWPs : dw->bpEditIdx;

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->cpu_radio_btn ) ) )
				{
					type |= BT_C;
				}
				else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->ppu_radio_btn ) ) )
				{
					type |= BT_P;
				}
				else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->sprite_radio_btn ) ) )
				{
					type |= BT_S;
				}

				txt = gtk_entry_get_text( GTK_ENTRY( dw->bp_start_entry ) );

				if ( txt[0] != 0 )
				{
					start_addr = offsetStringToInt( type, txt );
				}

				txt = gtk_entry_get_text( GTK_ENTRY( dw->bp_end_entry ) );

				if ( txt[0] != 0 )
				{
					end_addr = offsetStringToInt( type, txt );
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_read_chkbox ) ) )
				{
					type |= WP_R;
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_write_chkbox ) ) )
				{
					type |= WP_W;
				}

				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_execute_chkbox ) ) )
				{
					type |= WP_X;
				}

				//this overrides all
				if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_forbid_chkbox ) ) )
				{
					type  = WP_F;
				}

				enable = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( dw->bp_enable_chkbox ) );

				name = gtk_entry_get_text( GTK_ENTRY( dw->bp_name_entry ) );
				cond = gtk_entry_get_text( GTK_ENTRY( dw->bp_cond_entry ) );

				if ( (start_addr >= 0) && (numWPs < 64) )
				{
					unsigned int retval;

					retval = NewBreak( name, start_addr, end_addr, type, cond, slot, enable);

					if ( (retval == 1) || (retval == 2) )
					{
						printf("Breakpoint Add Failed\n");
					}
					else
					{
						if (dw->dialog_op == 1)
						{
							numWPs++;
						}

						dw->bpListUpdate();
					}
				}
			}
			break;
			default:
				// Do Nothing
			break;
		}
	}
	//else if ( response_id == GTK_RESPONSE_CANCEL )
	//{
	//	printf("Reponse Cancel\n");
	//}
	gtk_widget_destroy (w);

}

static void
tree_select_rowCB (GtkTreeView *treeview,
               	debuggerWin_t * dw )
{
	int row, row_is_selected;

	row = dw->get_bpList_selrow();

	row_is_selected = (row >= 0);

	//printf("Selected row = %i\n", row);

	gtk_widget_set_sensitive( dw->del_bp_button , row_is_selected );
	gtk_widget_set_sensitive( dw->edit_bp_button, row_is_selected );
}

static void closeDialogWindow (GtkWidget * w, GdkEvent * e, debuggerWin_t * dw)
{
	gtk_widget_destroy (w);
}

static void create_breakpoint_dialog( int index, watchpointinfo *wp, debuggerWin_t * dw )
{
	GtkWidget *win;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *vbox1;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *grid;
	char stmp[64];

	if ( index < 0 )
	{
		dw->dialog_op = 1;

		win = gtk_dialog_new_with_buttons ("Add Breakpoint",
						       GTK_WINDOW (dw->win),
						       (GtkDialogFlags)
						       (GTK_DIALOG_DESTROY_WITH_PARENT),
						       "_Cancel", GTK_RESPONSE_CANCEL,
						       "_Add", GTK_RESPONSE_OK, NULL);
	}
	else
	{
		dw->dialog_op = 2;

		win = gtk_dialog_new_with_buttons ("Edit Breakpoint",
						       GTK_WINDOW (dw->win),
						       (GtkDialogFlags)
						       (GTK_DIALOG_DESTROY_WITH_PARENT),
						       "_Cancel", GTK_RESPONSE_CANCEL,
						       "_Edit", GTK_RESPONSE_OK, NULL);

		dw->bpEditIdx = index;
	}

	gtk_dialog_set_default_response( GTK_DIALOG(win), GTK_RESPONSE_OK );

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

	// Adress entry fields
	label = gtk_label_new("Address:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->bp_start_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->bp_start_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->bp_start_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_start_entry, TRUE, TRUE, 2);

	label = gtk_label_new("-");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->bp_end_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->bp_end_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->bp_end_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_end_entry, TRUE, TRUE, 2);

	dw->bp_forbid_chkbox = gtk_check_button_new_with_label("Forbid");
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_forbid_chkbox, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);

	// flags frame
	frame = gtk_frame_new ("");
	gtk_box_pack_start (GTK_BOX (vbox1), frame, FALSE, FALSE, 2);

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

	dw->bp_read_chkbox    = gtk_check_button_new_with_label("Read");
	dw->bp_write_chkbox   = gtk_check_button_new_with_label("Write");
	dw->bp_execute_chkbox = gtk_check_button_new_with_label("Execute");
	dw->bp_enable_chkbox  = gtk_check_button_new_with_label("Enable");

	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_read_chkbox   , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_write_chkbox  , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_execute_chkbox, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->bp_enable_chkbox , FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox), hbox , FALSE, FALSE, 2);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	frame = gtk_frame_new ("Memory");
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 2);

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);

	dw->cpu_radio_btn    = gtk_radio_button_new_with_label (NULL, "CPU");
	dw->ppu_radio_btn    = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(dw->cpu_radio_btn), "PPU");
	dw->sprite_radio_btn = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(dw->cpu_radio_btn), "Sprite");

	gtk_box_pack_start (GTK_BOX (hbox), dw->cpu_radio_btn   , TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->ppu_radio_btn   , TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), dw->sprite_radio_btn, TRUE, TRUE, 2);

	gtk_container_add (GTK_CONTAINER (frame), hbox);

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	label = gtk_label_new("Condition:");
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1 );

	label = gtk_label_new("Name:");
	gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1 );

	dw->bp_cond_entry = gtk_entry_new ();
	dw->bp_name_entry = gtk_entry_new ();

	gtk_grid_attach (GTK_GRID (grid), dw->bp_cond_entry, 1, 0, 3, 1 );
	gtk_grid_attach (GTK_GRID (grid), dw->bp_name_entry, 1, 1, 3, 1 );

	gtk_box_pack_start (GTK_BOX (vbox1), grid, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (win))), vbox1, TRUE, TRUE, 1);

	gtk_widget_show_all (win);

	g_signal_connect (win, "delete-event",
			  G_CALLBACK (closeDialogWindow), dw);
	g_signal_connect (win, "response",
			  G_CALLBACK (handleDialogResponse), dw);

	if ( wp != NULL )
	{  // Sync entries to passed in breakpoint
		if ( wp->flags & BT_P )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->ppu_radio_btn ), TRUE );
		}
		else if ( wp->flags & BT_S )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->sprite_radio_btn ), TRUE );
		}
		else
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->cpu_radio_btn ), TRUE );
		}

		sprintf( stmp, "%04X", wp->address );

		gtk_entry_set_text (GTK_ENTRY (dw->bp_start_entry), stmp );

		if ( wp->endaddress > 0 )
		{
			sprintf( stmp, "%04X", wp->endaddress );

			gtk_entry_set_text (GTK_ENTRY (dw->bp_end_entry), stmp );
		}

		if ( wp->flags & WP_R )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_read_chkbox ), TRUE );
		}
		if ( wp->flags & WP_W )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_write_chkbox ), TRUE );
		}
		if ( wp->flags & WP_X )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_execute_chkbox ), TRUE );
		}
		if ( wp->flags & WP_F )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_forbid_chkbox ), TRUE );
		}
		if ( wp->flags & WP_E )
		{
		   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_enable_chkbox ), TRUE );
		}

		if ( wp->condText )
		{
			gtk_entry_set_text (GTK_ENTRY (dw->bp_cond_entry), wp->condText );
		}

		if ( wp->desc )
		{
			gtk_entry_set_text (GTK_ENTRY (dw->bp_name_entry), wp->desc );
		}
	}
	else
	{  // New entry, fill in current PC
		sprintf( stmp, "%04X", X.PC );

		gtk_entry_set_text (GTK_ENTRY (dw->bp_start_entry), stmp );

	   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_enable_chkbox ), TRUE );
	   gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dw->bp_execute_chkbox ), TRUE );
	}
}

static void addBreakpointCB (GtkButton * button, debuggerWin_t * dw)
{
	create_breakpoint_dialog( -1, NULL, dw );
}

static void editBreakpointCB (GtkButton * button, debuggerWin_t * dw)
{
	int selRow;

	selRow = dw->get_bpList_selrow();

	if ( selRow >= 0 )
	{
		create_breakpoint_dialog( selRow, &watchpoint[selRow], dw );
	}
}

static void enableBreakpointCB (GtkCellRendererToggle * renderer,
				   gchar * pathStr, debuggerWin_t * dw)
{
	GtkTreePath *path;
	int depth;
	int *indexArray;

	path = gtk_tree_path_new_from_string (pathStr);

	if (path == NULL)
	{
		printf ("Error: gtk_tree_path_new_from_string failed\n");
		return;
	}

	depth = gtk_tree_path_get_depth (path);
	indexArray = gtk_tree_path_get_indices (path);

	if (depth > 0)
	{
		if ( !gtk_cell_renderer_toggle_get_active(renderer) )
		{
			//printf("Toggle: %i On\n", indexArray[0] );
		   watchpoint[indexArray[0]].flags |=  WP_E;
		}
		else
		{
			//printf("Toggle: %i Off\n", indexArray[0] );
		   watchpoint[indexArray[0]].flags &= ~WP_E;
		}
	}

	gtk_tree_path_free (path);

	dw->bpListUpdate();
}

static void DeleteBreak(int sel)
{
	if(sel<0) return;
	if(sel>=numWPs) return;
	if (watchpoint[sel].cond)
	{
		freeTree(watchpoint[sel].cond);
	}
	if (watchpoint[sel].condText)
	{
		free(watchpoint[sel].condText);
	}
	if (watchpoint[sel].desc)
	{
		free(watchpoint[sel].desc);
	}
	// move all BP items up in the list
	for (int i = sel; i < numWPs; i++) 
	{
		watchpoint[i].address = watchpoint[i+1].address;
		watchpoint[i].endaddress = watchpoint[i+1].endaddress;
		watchpoint[i].flags = watchpoint[i+1].flags;
// ################################## Start of SP CODE ###########################
		watchpoint[i].cond = watchpoint[i+1].cond;
		watchpoint[i].condText = watchpoint[i+1].condText;
		watchpoint[i].desc = watchpoint[i+1].desc;
// ################################## End of SP CODE ###########################
	}
	// erase last BP item
	watchpoint[numWPs].address = 0;
	watchpoint[numWPs].endaddress = 0;
	watchpoint[numWPs].flags = 0;
	watchpoint[numWPs].cond = 0;
	watchpoint[numWPs].condText = 0;
	watchpoint[numWPs].desc = 0;
	numWPs--;
}

static void deleteBreakpointCB (GtkButton * button, debuggerWin_t * dw)
{
	int selRow;

	selRow = dw->get_bpList_selrow();

	if ( (selRow >= 0) && (selRow < numWPs) )
	{
		DeleteBreak( selRow );
		dw->bpListUpdate();
	}
}

static void debugRunCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused()) 
	{
		dw->setRegsFromEntry();
		FCEUI_ToggleEmulationPause();
		//DebuggerWasUpdated = false done in above function;
	}
}

static void debugStepIntoCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused())
	{
		dw->setRegsFromEntry();
	}
	FCEUI_Debugger().step = true;
	FCEUI_SetEmulationPaused(0);
}

static void debugStepOutCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused() > 0) 
	{
		DebuggerState &dbgstate = FCEUI_Debugger();
		dw->setRegsFromEntry();
		if (dbgstate.stepout)
		{
			printf("Step Out is currently in process.\n");
			return;
		}
		if (GetMem(X.PC) == 0x20)
		{
			dbgstate.jsrcount = 1;
		}
		else 
		{
			dbgstate.jsrcount = 0;
		}
		dbgstate.stepout = 1;
		FCEUI_SetEmulationPaused(0);
	}
}

static void debugStepOverCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused()) 
	{
		dw->setRegsFromEntry();
		int tmp=X.PC;
		uint8 opcode = GetMem(X.PC);
		bool jsr = opcode==0x20;
		bool call = jsr;
		#ifdef BRK_3BYTE_HACK
		//with this hack, treat BRK similar to JSR
		if(opcode == 0x00)
		{
			call = true;
		}
		#endif
		if (call) 
		{
			if (watchpoint[64].flags)
			{
				printf("Step Over is currently in process.\n");
				return;
			}
			watchpoint[64].address = (tmp+3);
			watchpoint[64].flags = WP_E|WP_X;
		}
		else 
		{
			FCEUI_Debugger().step = true;
		}
		FCEUI_SetEmulationPaused(0);
	}
}
static void debugRunLineCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused())
	{
		dw->setRegsFromEntry();
	}
	uint64 ts=timestampbase;
	ts+=timestamp;
	ts+=341/3;
	//if (scanline == 240) vblankScanLines++;
	//else vblankScanLines = 0;
	FCEUI_Debugger().runline = true;
	FCEUI_Debugger().runline_end_time=ts;
	FCEUI_SetEmulationPaused(0);
}

static void debugRunLine128CB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused())
	{
		dw->setRegsFromEntry();
	}
	FCEUI_Debugger().runline = true;
	{
		uint64 ts=timestampbase;
		ts+=timestamp;
		ts+=128*341/3;
		FCEUI_Debugger().runline_end_time=ts;
		//if (scanline+128 >= 240 && scanline+128 <= 257) vblankScanLines = (scanline+128)-240;
		//else vblankScanLines = 0;
	}
	FCEUI_SetEmulationPaused(0);
}

static void seekPCCB (GtkButton * button, debuggerWin_t * dw)
{
	if (FCEUI_EmulationPaused())
	{
		dw->setRegsFromEntry();
		updateAllDebugWindows();
	}
	dw->seekAsmPC();
}

static void seekToCB (GtkButton * button, debuggerWin_t * dw)
{
	int addr;
	const char *txt;

	if (FCEUI_EmulationPaused())
	{
		dw->setRegsFromEntry();
	}
	txt = gtk_entry_get_text( GTK_ENTRY(dw->seektoEntry) );

	addr = offsetStringToInt( BT_C, txt );

	dw->seekAsmAddr(addr);
}

static void Flag_N_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= N_FLAG;
}
static void Flag_V_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= V_FLAG;
}
static void Flag_U_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= U_FLAG;
}
static void Flag_B_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= B_FLAG;
}
static void Flag_D_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= D_FLAG;
}
static void Flag_I_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= I_FLAG;
}
static void Flag_Z_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= Z_FLAG;
}
static void Flag_C_CB (GtkToggleButton * button, debuggerWin_t * dw)
{
	X.P ^= C_FLAG;
}

static void
addBreakpointMenuCB (GtkMenuItem *menuitem,
				      	debuggerWin_t * dw)
{
	watchpointinfo wp;

	wp.address = dw->ctx_menu_addr;
	wp.endaddress = 0;
	wp.flags   = WP_X | WP_E;
	wp.condText = 0;
	wp.desc = NULL;

	create_breakpoint_dialog( -1, &wp, dw );

}

static gboolean
populate_context_menu (GtkWidget *popup,
               debuggerWin_t * dw )
{
   GtkWidget *menu = NULL;
   GtkWidget *item;
	GtkTextIter iter;
	char stmp[256];
	gint cpos, lineNum;
	int addr;

	g_object_get( dw->textbuf, "cursor-position", &cpos, NULL );

	gtk_text_buffer_get_iter_at_offset( dw->textbuf, &iter, cpos );

	lineNum = gtk_text_iter_get_line( &iter );
	
	if ( (lineNum < 0) || (lineNum >= (int)dw->asmEntry.size() ) )
	{
		return TRUE;
	}
	dw->ctx_menu_addr = addr = dw->asmEntry[lineNum]->addr;

   //printf("Context Menu:  CP:%i  Line:%i  Addr:0x%04X\n", cpos, lineNum, addr );
	
	menu = gtk_menu_new ();
	
	sprintf( stmp, "Add Breakpoint at 0x%04X", addr );

	item = gtk_menu_item_new_with_label( stmp );

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate",
		G_CALLBACK (addBreakpointMenuCB), dw);

   gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL );
	//gtk_widget_show_all (popup);
	gtk_widget_show_all (menu);

   return TRUE;
}

static gboolean textview_keypress_cb (GtkWidget * grab, GdkEventKey * event,  debuggerWin_t * dw)
{
	gboolean stopKeyPropagate;

	stopKeyPropagate = (event->keyval != GDK_KEY_Up       ) &&
		                (event->keyval != GDK_KEY_Down     ) &&
		                (event->keyval != GDK_KEY_Left     ) &&
		                (event->keyval != GDK_KEY_Right    ) &&
		                (event->keyval != GDK_KEY_Page_Up  ) &&
		                (event->keyval != GDK_KEY_Page_Down);

	return stopKeyPropagate;
}

static gboolean
textview_button_press_cb (GtkWidget *widget,
               GdkEventButton  *event,
               debuggerWin_t * dw )
{
   gboolean  ret = FALSE;
   //printf("Press Button  %i   %u\n", event->type, event->button );

   if ( event->button == 3 )
   {
      ret = populate_context_menu( widget, dw );
   }

   return ret;
}

static void resetCountersCB( GtkWidget *button, debuggerWin_t * dw)
{
	ResetDebugStatisticsCounters();

	updateAllDebugWindows();
}

static void breakOnBadOpcodeCB( GtkToggleButton *togglebutton, debuggerWin_t * dw)
{
	FCEUI_Debugger().badopbreak = !FCEUI_Debugger().badopbreak;

	updateAllDebugWindows();
}

static void breakOnCyclesCB( GtkToggleButton *togglebutton, debuggerWin_t * dw)
{
	const char *txt;

	break_on_cycles = !break_on_cycles;

	txt = gtk_entry_get_text( GTK_ENTRY(dw->brk_cycles_lim_entry) );

   //printf("'%s'\n", txt );

	if ( isdigit(txt[0]) )
	{
		break_cycles_limit = strtoul( txt, NULL, 0 );
	}

	updateAllDebugWindows();
}

static void
breakOnCyclesLimitCB (GtkEntry *entry,
//                   gchar    *string,
                     gpointer  user_data)
{
	const char *txt;

	txt = gtk_entry_get_text( entry );

   //printf("'%s'\n", txt );

	if ( isdigit(txt[0]) )
	{
		break_cycles_limit = strtoul( txt, NULL, 0 );
	}
	updateAllDebugWindows();
}

static void breakOnInstructionsCB( GtkToggleButton *togglebutton, debuggerWin_t * dw)
{
	const char *txt;

	break_on_instructions = !break_on_instructions;

	txt = gtk_entry_get_text( GTK_ENTRY(dw->brk_instrs_lim_entry) );

   //printf("'%s'\n", txt );

	if ( isdigit(txt[0]) )
	{
		break_instructions_limit = strtoul( txt, NULL, 0 );
	}

	updateAllDebugWindows();
}

static void
breakOnInstructionsLimitCB (GtkEntry *entry,
//                   gchar    *string,
                     gpointer  user_data)
{
	const char *txt;

	txt = gtk_entry_get_text( entry );

   //printf("'%s'\n", txt );

	if ( isdigit(txt[0]) )
	{
		break_instructions_limit = strtoul( txt, NULL, 0 );
	}
	updateAllDebugWindows();
}

static void romOffsetToggleCB( GtkToggleButton *togglebutton, debuggerWin_t * dw)
{
	dw->displayROMoffsets = gtk_toggle_button_get_active( togglebutton );

	dw->updateViewPort();

	//printf("Toggle ROM Offset: %i \n", dw->displayROMoffsets );
}

static void updateAllDebugWindows(void)
{
	std::list < debuggerWin_t * >::iterator it;

	for (it = debuggerWinList.begin (); it != debuggerWinList.end (); it++)
	{
		(*it)->updateViewPort();
	}
}

static void seekPC_AllDebugWindows(void)
{
	std::list < debuggerWin_t * >::iterator it;

	for (it = debuggerWinList.begin (); it != debuggerWinList.end (); it++)
	{
		(*it)->seekAsmPC();
	}
}

static void winDebuggerLoopStep(void)
{
	FCEUD_UpdateInput();

	while(gtk_events_pending())
	{
		gtk_main_iteration_do(FALSE);
	}

	if ( XBackBuf )
	{
		BlitScreen(XBackBuf);
	}

	if ( breakpoint_hit )
	{
		seekPC_AllDebugWindows();
		breakpoint_hit = 0;
	}
}

//this code enters the debugger when a breakpoint was hit
void FCEUD_DebugBreakpoint(int bp_num)
{
	//printf("Breakpoint Hit: %i \n", bp_num);
	
	breakpoint_hit = 1;
	
	updateAllDebugWindows();

	winDebuggerLoopStep();

	while(FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
	{
		usleep(50000);
		winDebuggerLoopStep();
	}
}

void FCEUD_TraceInstruction(uint8 *opcode, int size)
{
   // Place holder to allow for compiling. GTK GUI doesn't support this. Qt Does.
}

void FCEUD_UpdatePPUView(int scanline, int refreshchr)
{
   // Place holder to allow for compiling. GTK GUI doesn't support this. Qt Does.
}

void FCEUD_UpdateNTView(int scanline, bool drawall)
{
   // Place holder to allow for compiling. GTK GUI doesn't support this. Qt Does.
}

static void closeDebuggerWindow (GtkWidget * w, GdkEvent * e, debuggerWin_t * dw)
{
	std::list < debuggerWin_t * >::iterator it;

	for (it = debuggerWinList.begin (); it != debuggerWinList.end (); it++)
	{
		if (dw == *it)
		{
			debuggerWinList.erase (it);
			break;
		}
	}

	delete dw;

	gtk_widget_destroy (w);
}

void openDebuggerWindow (void)
{
	GtkWidget *main_hbox;
	GtkWidget *vbox1, *vbox2, *vbox3;
	GtkWidget *hbox1, *hbox2;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkWidget *frame;
	GtkWidget *entry;
	GtkWidget *grid;
	GtkCellRenderer *renderer;
	GtkCellRenderer *chkbox_renderer;
	GtkTreeViewColumn *column;
	debuggerWin_t *dw;

	dw = new debuggerWin_t;

	debuggerWinList.push_back (dw);

	dw->win = gtk_dialog_new_with_buttons ("6502 Debugger",
						GTK_WINDOW (MainWindow),
						(GtkDialogFlags)
						(GTK_DIALOG_DESTROY_WITH_PARENT),
						"_Close", GTK_RESPONSE_OK,
						NULL);

	gtk_window_set_default_size (GTK_WINDOW (dw->win), 800, 800);

	main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	dw->textview = (GtkTextView*) gtk_text_view_new();

	gtk_text_view_set_monospace( dw->textview, TRUE );
	gtk_text_view_set_overwrite( dw->textview, TRUE );
	gtk_text_view_set_editable( dw->textview, TRUE );
	gtk_text_view_set_wrap_mode( dw->textview, GTK_WRAP_NONE );
	gtk_text_view_set_cursor_visible( dw->textview, TRUE );
	//gtk_widget_set_size_request( GTK_WIDGET(dw->textview), 400, 400 );

	g_signal_connect (dw->textview, "key-press-event",
			  G_CALLBACK (textview_keypress_cb), dw);
	g_signal_connect (dw->textview, "button-press-event",
			  G_CALLBACK (textview_button_press_cb), dw);

	dw->textbuf = gtk_text_view_get_buffer( dw->textview );

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					//GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
					GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(dw->textview) );

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 2);

	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 2);

	/*
	 *  Vertical box 2 contains:
	 *     1. "Run" and "Step Into" Buttons
	 *     2. "Step Out" and "Step Over" Buttons
	 *     3. "Run Line" and "128 Lines" Buttons
	 *     4. "Seek To:" button and Address Entry Field
	 *     5. PC Entry Field and "Seek PC" Button
	 *     6. A, X, Y Register Entry Fields
	 */
	vbox2  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	hbox   = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 2);

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	gtk_grid_insert_column( GTK_GRID(grid), 0 );
	gtk_grid_insert_column( GTK_GRID(grid), 0 );

	for (int i=0; i<5;i++)
	{
		gtk_grid_insert_row( GTK_GRID(grid), 0 );
	}
	gtk_box_pack_start (GTK_BOX (hbox ), grid, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 2);

	//     Row 1
	button = gtk_button_new_with_label ("Run");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugRunCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 0, 0, 1, 1 );

	button = gtk_button_new_with_label ("Step Into");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugStepIntoCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 1, 0, 1, 1 );

	//     Row 2
	button = gtk_button_new_with_label ("Step Out");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugStepOutCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 0, 1, 1, 1 );

	button = gtk_button_new_with_label ("Step Over");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugStepOverCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 1, 1, 1, 1 );

	//     Row 3
	button = gtk_button_new_with_label ("Run Line");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugRunLineCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 0, 2, 1, 1 );

	button = gtk_button_new_with_label ("128 Lines");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (debugRunLine128CB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 1, 2, 1, 1 );

	//     Row 4
	button = gtk_button_new_with_label ("Seek To:");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (seekToCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 0, 3, 1, 1 );

	dw->seektoEntry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->seektoEntry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->seektoEntry), 4);

	gtk_grid_attach( GTK_GRID(grid), dw->seektoEntry, 1, 3, 1, 1 );

	//     Row 5
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("PC:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->pc_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->pc_entry), 4);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->pc_entry), 4);

	gtk_box_pack_start (GTK_BOX (hbox), dw->pc_entry, TRUE, TRUE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 0, 4, 1, 1 );

	button = gtk_button_new_with_label ("Seek PC");

	g_signal_connect (button, "clicked",
			  G_CALLBACK (seekPCCB), (gpointer) dw);

	gtk_grid_attach( GTK_GRID(grid), button, 1, 4, 1, 1 );

	//     Row 6
	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("A:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->A_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->A_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->A_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->A_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 0, 0, 1, 1 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("X:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->X_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->X_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->X_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->X_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 1, 0, 1, 1 );

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	label = gtk_label_new("Y:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

	dw->Y_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (dw->Y_entry), 2);
	gtk_entry_set_width_chars (GTK_ENTRY (dw->Y_entry), 2);

	gtk_box_pack_start (GTK_BOX (hbox), dw->Y_entry, FALSE, FALSE, 2);

	gtk_grid_attach( GTK_GRID(grid), hbox, 2, 0, 1, 1 );

	gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 2);

	// Stack Frame
	dw->stack_frame = gtk_frame_new ("Stack");

	dw->stackview = (GtkTextView*) gtk_text_view_new();

	gtk_text_view_set_monospace( dw->stackview, TRUE );
	gtk_text_view_set_overwrite( dw->stackview, TRUE );
	gtk_text_view_set_editable( dw->stackview, FALSE );
	gtk_text_view_set_wrap_mode( dw->stackview, GTK_WRAP_NONE );
	gtk_text_view_set_cursor_visible( dw->stackview, TRUE );

	dw->stackTextBuf = gtk_text_view_get_buffer( dw->stackview );

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(dw->stackview) );

	gtk_container_add (GTK_CONTAINER (dw->stack_frame), scroll);

	gtk_box_pack_start (GTK_BOX (vbox2), dw->stack_frame, TRUE, TRUE, 2);
	/*
	 *  Vertical box 3 contains:
	 *     1. Breakpoints Frame
	 *     2. Status Flags Frame
	 */
	vbox3  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	gtk_box_pack_start (GTK_BOX (hbox1), vbox3, TRUE, TRUE, 2);

	/*
	 *  Breakpoints Frame contains:
	 *     1. Breakpoints tree view
	 *     2. Add, delete, edit breakpoint button row.
	 *     3. Break on bad opcodes checkbox/label
	 */
	frame = gtk_frame_new ("Breakpoints");

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (vbox3), frame, TRUE, TRUE, 2);

	dw->bp_store =
		gtk_tree_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING );

	dw->bp_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (dw->bp_store));

	g_signal_connect (dw->bp_tree, "cursor-changed",
			  G_CALLBACK (tree_select_rowCB), (gpointer) dw);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (dw->bp_tree),
				      GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	chkbox_renderer = gtk_cell_renderer_toggle_new ();
	gtk_cell_renderer_toggle_set_activatable ((GtkCellRendererToggle *)
						  chkbox_renderer, TRUE);
	column = gtk_tree_view_column_new_with_attributes ("Ena",
							   chkbox_renderer,
							   "active", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dw->bp_tree), column);

	g_signal_connect (chkbox_renderer, "toggled",
			  G_CALLBACK (enableBreakpointCB), (void *) dw);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Addr/Flags", renderer,
							   "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dw->bp_tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), dw->bp_tree);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 1);

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

	dw->add_bp_button = gtk_button_new_with_label ("Add");

	g_signal_connect (dw->add_bp_button, "clicked",
			  G_CALLBACK (addBreakpointCB), (gpointer) dw);

	gtk_box_pack_start (GTK_BOX (hbox), dw->add_bp_button, TRUE, TRUE, 2);

	dw->del_bp_button = gtk_button_new_with_label ("Delete");

	gtk_widget_set_sensitive( dw->del_bp_button, FALSE );

	g_signal_connect (dw->del_bp_button, "clicked",
			  G_CALLBACK (deleteBreakpointCB), (gpointer) dw);

	gtk_box_pack_start (GTK_BOX (hbox), dw->del_bp_button, TRUE, TRUE, 2);

	dw->edit_bp_button = gtk_button_new_with_label ("Edit");

	gtk_widget_set_sensitive( dw->edit_bp_button, FALSE );

	g_signal_connect (dw->edit_bp_button, "clicked",
			  G_CALLBACK (editBreakpointCB), (gpointer) dw);

	gtk_box_pack_start (GTK_BOX (hbox), dw->edit_bp_button, TRUE, TRUE, 2);

	dw->badop_chkbox = gtk_check_button_new_with_label("Break on Bad Opcodes");
	g_signal_connect (dw->badop_chkbox, "toggled",
			  G_CALLBACK (breakOnBadOpcodeCB), dw);
	gtk_box_pack_start (GTK_BOX (vbox), dw->badop_chkbox, FALSE, FALSE, 1);

	// Status Flags Frame
	frame = gtk_frame_new ("Status Flags");

	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	dw->P_N_chkbox = gtk_check_button_new_with_label("N");
	g_signal_connect (dw->P_N_chkbox, "toggled",
			  G_CALLBACK (Flag_N_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_N_chkbox, 0, 0, 1, 1 );

	dw->P_V_chkbox = gtk_check_button_new_with_label("V");
	g_signal_connect (dw->P_V_chkbox, "toggled",
			  G_CALLBACK (Flag_V_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_V_chkbox, 1, 0, 1, 1 );

	dw->P_U_chkbox = gtk_check_button_new_with_label("U");
	g_signal_connect (dw->P_U_chkbox, "toggled",
			  G_CALLBACK (Flag_U_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_U_chkbox, 2, 0, 1, 1 );

	dw->P_B_chkbox = gtk_check_button_new_with_label("B");
	g_signal_connect (dw->P_B_chkbox, "toggled",
			  G_CALLBACK (Flag_B_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_B_chkbox, 3, 0, 1, 1 );

	dw->P_D_chkbox = gtk_check_button_new_with_label("D");
	g_signal_connect (dw->P_D_chkbox, "toggled",
			  G_CALLBACK (Flag_D_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_D_chkbox, 0, 1, 1, 1 );

	dw->P_I_chkbox = gtk_check_button_new_with_label("I");
	g_signal_connect (dw->P_I_chkbox, "toggled",
			  G_CALLBACK (Flag_I_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_I_chkbox, 1, 1, 1, 1 );

	dw->P_Z_chkbox = gtk_check_button_new_with_label("Z");
	g_signal_connect (dw->P_Z_chkbox, "toggled",
			  G_CALLBACK (Flag_Z_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_Z_chkbox, 2, 1, 1, 1 );

	dw->P_C_chkbox = gtk_check_button_new_with_label("C");
	g_signal_connect (dw->P_C_chkbox, "toggled",
			  G_CALLBACK (Flag_C_CB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->P_C_chkbox, 3, 1, 1, 1 );

	gtk_container_add (GTK_CONTAINER (frame), grid);

	gtk_box_pack_start (GTK_BOX (vbox3), frame, FALSE, FALSE, 2);

	/*
	 *  End Vertical Box 3
	 */

	/*
	 *  Start PPU Frame
	 */
	grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_column_homogeneous( GTK_GRID(grid), TRUE );
	gtk_grid_set_row_spacing( GTK_GRID(grid), 5 );
	gtk_grid_set_column_spacing( GTK_GRID(grid), 10 );

	frame = gtk_frame_new ("");

	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

	dw->ppu_label      = gtk_label_new("PPU:");
	dw->sprite_label   = gtk_label_new("Sprite:");
	dw->scanline_label = gtk_label_new("Scanline:");
	dw->pixel_label    = gtk_label_new("Pixel:");

	gtk_box_pack_start (GTK_BOX (vbox), dw->ppu_label     , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->sprite_label  , FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->scanline_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), dw->pixel_label   , FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	gtk_grid_attach( GTK_GRID(grid), frame, 0, 0, 1, 4 );
	/*
	 *  End PPU Frame
	 */

	/*
	 *  Start Cycle Break Block
	 */
	dw->cpu_label1      = gtk_label_new("CPU Cycles:");
	dw->cpu_label2      = gtk_label_new("+(0)");

	dw->instr_label1      = gtk_label_new("Instructions:");
	dw->instr_label2      = gtk_label_new("+(0)");

	gtk_grid_attach( GTK_GRID(grid), dw->cpu_label1, 1, 0, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->cpu_label2, 2, 0, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->instr_label1, 1, 2, 1, 1 );
	gtk_grid_attach( GTK_GRID(grid), dw->instr_label2, 2, 2, 1, 1 );

	dw->brkcycles_chkbox = gtk_check_button_new_with_label("Break when exceed");
	g_signal_connect (dw->brkcycles_chkbox, "toggled",
			  G_CALLBACK (breakOnCyclesCB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->brkcycles_chkbox, 1, 1, 1, 1 );
	dw->brk_cycles_lim_entry = gtk_entry_new ();
	//g_signal_connect (dw->brk_cycles_lim_entry, "preedit-changed",
	g_signal_connect (dw->brk_cycles_lim_entry, "activate",
			  G_CALLBACK (breakOnCyclesLimitCB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->brk_cycles_lim_entry, 2, 1, 1, 1 );

	dw->brkinstrs_chkbox = gtk_check_button_new_with_label("Break when exceed");
	g_signal_connect (dw->brkinstrs_chkbox, "toggled",
			  G_CALLBACK (breakOnInstructionsCB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->brkinstrs_chkbox, 1, 3, 1, 1 );
	dw->brk_instrs_lim_entry = gtk_entry_new ();
	g_signal_connect (dw->brk_instrs_lim_entry, "activate",
			  G_CALLBACK (breakOnInstructionsLimitCB), dw);
	gtk_grid_attach( GTK_GRID(grid), dw->brk_instrs_lim_entry, 2, 3, 1, 1 );

	gtk_box_pack_start (GTK_BOX (vbox1), grid, FALSE, FALSE, 2);
	/*
	 *  End Cycle Break Block
	 */

	/*
	 *  Start Address Bookmarks
	 */
	hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	frame = gtk_frame_new ("Address Bookmarks");

	vbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
	hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 1);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	dw->bkm_store =
		gtk_tree_store_new (1, G_TYPE_STRING );

	dw->bkm_tree =
		gtk_tree_view_new_with_model (GTK_TREE_MODEL
					      (dw->bkm_store));

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "family", "MonoSpace", NULL);
	column = gtk_tree_view_column_new_with_attributes ("Bookmarks", renderer,
							   "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dw->bkm_tree), column);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), dw->bkm_tree);
	gtk_box_pack_start (GTK_BOX (hbox), scroll, TRUE, TRUE, 1);

	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 1);
	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Add");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Delete");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Name");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, 2);
	
	vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start (GTK_BOX (hbox2), vbox, FALSE, FALSE, 2);

	button = gtk_button_new_with_label ("Reset Counters");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (resetCountersCB), dw);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("ROM Offsets");
	g_signal_connect (button, "toggled",
			  G_CALLBACK (romOffsetToggleCB), dw);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("Symbolic Debug");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_check_button_new_with_label("Register Names");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("Reload Symbols");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
	button = gtk_button_new_with_label ("ROM Patcher");
	gtk_widget_set_sensitive( button , FALSE ); // TODO Insensitive until functionality implemented
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 2);
	/*
	 *  End Address Bookmarks
	 */
	gtk_box_pack_start (GTK_BOX (main_hbox), vbox1, TRUE, TRUE, 2);

	gtk_box_pack_start (GTK_BOX
			    (gtk_dialog_get_content_area
			     (GTK_DIALOG (dw->win))), main_hbox, TRUE, TRUE,
			    0);

	g_signal_connect (dw->win, "delete-event",
			  G_CALLBACK (closeDebuggerWindow), dw);
	g_signal_connect (dw->win, "response",
			  G_CALLBACK (closeDebuggerWindow), dw);

	gtk_widget_show_all (dw->win);

	dw->updateViewPort();

   return;
}
