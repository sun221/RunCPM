/* see main.c for definition */

#define NOP		0x00
#define JP		0xc3
#define RET		0xc9
#define INa		0xdb	// Triggers a BIOS call
#define OUTa	0xd3	// Triggers a BDOS call

uint8	LastSel;		// Last disk selected

void _PatchCPM(void)
{
	uint16 i;

	//**********  Patch CP/M page zero into the memory  **********

	/* BIOS entry point */
	_RamWrite(0x0000, JP);		/* JP BIOS+3 (warm boot) */
	_RamWrite16(0x0001, BIOSjmppage + 3);

	/* IOBYTE - Points to Console */
	_RamWrite(0x0003, 0x3D);

	/* Current drive/user - A:/0 */
	if (Status!=2)
		_RamWrite(0x0004, 0x00);

	/* BDOS entry point (0x0005) */
	_RamWrite(0x0005, JP);
	_RamWrite16(0x0006, BDOSjmppage + 0x06);

	//**********  Patch CP/M Version into the memory so the CCP can see it
	_RamWrite16(BDOSjmppage, 0x1600);
	_RamWrite16(BDOSjmppage + 2, 0x0000);
	_RamWrite16(BDOSjmppage + 4, 0x0000);

	// Patches in the BDOS jump destination
	_RamWrite(BDOSjmppage + 6, JP);
	_RamWrite16(BDOSjmppage + 7, BDOSpage);

	// Patches in the BDOS page content
	_RamWrite(BDOSpage, INa);
	_RamWrite(BDOSpage+1, 0x00);
	_RamWrite(BDOSpage+2, RET);

	// Patches in the BIOS jump destinations
	for (i = 0; i < 0x33; i = i + 3)
	{
		_RamWrite(BIOSjmppage + i, JP);
		_RamWrite16(BIOSjmppage + i + 1, BIOSpage + i);
	}

	// Patches in the BIOS page content
	for (i = 0; i < 0x33; i = i + 3)
	{
		_RamWrite(BIOSpage + i, OUTa);
		_RamWrite(BIOSpage + i + 1, i & 0xff);
		_RamWrite(BIOSpage + i + 2, RET);
	}

	//**********  Patch CP/M (fake) Disk Paramater Table after the BDOS call entry  **********
	i = DPBaddr;
	_RamWrite(i++, 0x20);		/* spt - Sectors Per Track */
	_RamWrite(i++, 0x00);
	_RamWrite(i++, 0x04);		/* bsh - Data allocation "Block Shift Factor" */
	_RamWrite(i++, 0x0f);		/* blm - Data allocation Block Mask */
	_RamWrite(i++, 0x00);		/* exm - Extent Mask */
	_RamWrite(i++, 0xff);        /* dsm - Total storage capacity of the disk drive */
	_RamWrite(i++, 0x01);
	_RamWrite(i++, 0xfe);		/* drm - Number of the last directory entry */
	_RamWrite(i++, 0x00);
	_RamWrite(i++, 0xF0);		/* al0 */
	_RamWrite(i++, 0x00);		/* al1 */
	_RamWrite(i++, 0x3f);		/* cks - Check area Size */
	_RamWrite(i++, 0x00);
	_RamWrite(i++, 0x02);		/* off - Number of system reserved tracks at the beginning of the ( logical ) disk */
	_RamWrite(i++, 0x00);

#ifdef PatchCCP
	_RamWrite(PatchCCP, (BDOSjmppage) >> 8);
#endif

}

#ifdef DEBUGLOG
void _logMemory(uint16 pos, uint8 size)
{
	FILE* file = _sys_fopen_a((uint8 *)"RunCPM.log");
	uint16 h = pos;
	uint16 c = pos;
	uint8 l, i;
	uint8 ch;
	uint8 nl = (size / 16) + 1;

//	fprintf(file, "\r\n\t\t");
	for (l = 0; l < nl; l++)
	{
		fprintf(file, "\t\t%04x : ", h);
		for (i = 0; i < 16; i++)
		{
			fprintf(file, "%02x ", _RamRead(h++));
		}
		for (i = 0; i < 16; i++)
		{
			ch = _RamRead(c++);
			fprintf(file, "%c",ch>31 && ch<127 ? ch : '.');
		}
		fprintf(file, "\r\n");
	}
	_sys_fclose(file);
}

char flags[9];
char *_logFlags(uint8 ch)
{
	flags[0] = (ch & 0b10000000) ? 'S' : '.';
	flags[1] = (ch & 0b01000000) ? 'Z' : '.';
	flags[2] = (ch & 0b00100000) ? '5' : '.';
	flags[3] = (ch & 0b00010000) ? 'H' : '.';
	flags[4] = (ch & 0b00001000) ? '3' : '.';
	flags[5] = (ch & 0b00000100) ? 'P' : '.';
	flags[6] = (ch & 0b00000010) ? 'N' : '.';
	flags[7] = (ch & 0b00000001) ? 'C' : '.';
	flags[8] = 0;
	return(&flags[0]);
}

void _logBiosIn(uint8 ch)
{
	FILE* file = _sys_fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "Bios call: %d (0x%02x) - ", ch, ch);
	switch (ch) {
	case 0:
		fputs("Cold Boot\r\n", file);
		break;
	case 3:
		fputs("Warm Boot\r\n", file);
		break;
	case 6:
		fputs("Console Status\r\n", file);
		break;
	case 9:
		fputs("Console Input\r\n", file);
		break;
	case 12:
		fputs("Console Output\r\n", file);
		break;
	case 15:
		fputs("List Output\r\n", file);
		break;
	case 18:
		fputs("Punch Output\r\n", file);
		break;
	case 21:
		fputs("Reader Input\r\n", file);
		break;
	case 24:
		fputs("Home Disk\r\n", file);
		break;
	case 27:
		fputs("Select Disk\r\n", file);
		break;
	case 30:
		fputs("Select Track\r\n", file);
		break;
	case 33:
		fputs("Select Sector\r\n", file);
		break;
	case 36:
		fputs("Set DMA Address\r\n", file);
		break;
	case 39:
		fputs("Read Selected Sector\r\n", file);
		break;
	case 42:
		fputs("Write Selected Sector\r\n", file);
		break;
	case 45:
		fputs("List Status\r\n", file);
		break;
	case 48:
		fputs("Sector Translation\r\n", file);
		break;
	default:
		fputs("Unknown\r\n", file);
		break;
	}
	fprintf(file, "\tIn : BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_sys_fclose(file);
}

void _logBiosOut(uint8 ch)
{
	FILE* file = _sys_fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "\tOut: BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_sys_fclose(file);
}

void _logBdosIn(uint8 ch)
{
	FILE* file = _sys_fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "Bdos call: %d (0x%02x) - ", ch, ch);
	switch (ch){
	case 0:
		fputs("System Reset\r\n", file);
		break;
	case 1:
		fputs("Console Input\r\n", file);
		break;
	case 2:
		fputs("Console Output\r\n", file);
		break;
	case 3:
		fputs("Reader Input\r\n", file);
		break;
	case 4:
		fputs("Punch Output\r\n", file);
		break;
	case 5:
		fputs("List Output\r\n", file);
		break;
	case 6:
		fputs("Direct Console I/O\r\n", file);
		break;
	case 7:
		fputs("Get I/O Byte\r\n", file);
		break;
	case 8:
		fputs("Set I/O Byte\r\n", file);
		break;
	case 9:
		fputs("Print String\r\n", file);
		break;
	case 10:
		fputs("Read Console Buffer\r\n", file);
		break;
	case 11:
		fputs("Get Console Status\r\n", file);
		break;
	case 12:
		fputs("Return Version Number\r\n", file);
		break;
	case 13:
		fputs("Reset Disk System\r\n", file);
		break;
	case 14:
		fputs("Select Disk\r\n", file);
		break;
	case 15:
		fputs("Open File\r\n", file);
		break;
	case 16:
		fputs("Close File\r\n", file);
		break;
	case 17:
		fputs("Search for First\r\n", file);
		break;
	case 18:
		fputs("Search for Next\r\n", file);
		break;
	case 19:
		fputs("Delete File\r\n", file);
		break;
	case 20:
		fputs("Read Sequential\r\n", file);
		break;
	case 21:
		fputs("Write Sequential\r\n", file);
		break;
	case 22:
		fputs("Make File\r\n", file);
		break;
	case 23:
		fputs("Rename File\r\n", file);
		break;
	case 24:
		fputs("Return Log-in Vector\r\n", file);
		break;
	case 25:
		fputs("Return Current Disk\r\n", file);
		break;
	case 26:
		fputs("Set DMA Address\r\n", file);
		break;
	case 27:
		fputs("Get Addr(Alloc)\r\n", file);
		break;
	case 28:
		fputs("Write Protect Disk\r\n", file);
		break;
	case 29:
		fputs("Get Read/Only Vector\r\n", file);
		break;
	case 30:
		fputs("Set File Attributes\r\n", file);
		break;
	case 31:
		fputs("Get ADDR(Disk Parms)\r\n", file);
		break;
	case 32:
		fputs("Set/Get User Code\r\n", file);
		break;
	case 33:
		fputs("Read Random\r\n", file);
		break;
	case 34:
		fputs("Write Random\r\n", file);
		break;
	case 35:
		fputs("Compute File Size\r\n", file);
		break;
	case 36:
		fputs("Set Random Record\r\n", file);
		break;
	case 37:
		fputs("Reset Drive\r\n", file);
		break;
	case 38:
		fputs("Access Drive (not supported)\r\n", file);
		break;
	case 39:
		fputs("Free Drive (not supported)\r\n", file);
		break;
	case 40:
		fputs("Write Random with Zero Fill\r\n", file);
		break;
#ifdef ARDUINO
	case 220:
		fputs("PinMode\r\n", file);
		break;
	case 221:
		fputs("DigitalRead\r\n", file);
		break;
	case 222:
		fputs("DigitalWrite\r\n", file);
		break;
	case 223:
		fputs("AnalogRead\r\n", file);
		break;
	case 224:
		fputs("AnalogWrite\r\n", file);
		break;
#endif
	default:
		fputs("Unknown\r\n", file);
		break;
	}
	fprintf(file, "\tIn : BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_sys_fclose(file);

	switch (ch){
	case 9:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 30:
	case 33:
	case 34:
	case 35:
	case 36:
	case 40:
		_logMemory(DE, 36);
	default:
		break;
	}
}

void _logBdosOut(uint8 ch)
{
	FILE* file = _sys_fopen_a((uint8 *)"RunCPM.log");
	fprintf(file, "\tOut: BC=%04x DE=%04x HL=%04x AF=%02x|%s| SP=%04x PC=%04x IOB=%02x DDR=%02x\r\n", BC, DE, HL, HIGH_REGISTER(AF), _logFlags(LOW_REGISTER(AF)), SP, PCX, _RamRead(3), _RamRead(4));
	_sys_fclose(file);

	switch (ch){
	case 10:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 30:
	case 33:
	case 34:
	case 35:
	case 36:
	case 40:
#ifdef ARDUINO
	case 220:
	case 221:
	case 222:
	case 223:
	case 224:
#endif
		_logMemory(DE, 36);
	default:
		break;
	}
}
#endif // DEBUGLOG

void _Bios(void)
{
	uint8 ch = LOW_REGISTER(PCX);

#ifdef DEBUGLOG
	_logBiosIn(ch);
#endif

	switch (ch) {
	case 0x00:
		Status = 1;			// 0 - BOOT - Ends RunCPM
		break;
	case 0x03:
		Status = 2;			// 1 - WBOOT - Back to CCP
		break;
	case 0x06:				// 2 - CONST - Console status
		SET_HIGH_REGISTER(AF, _chready());
		break;
	case 0x09:				// 3 - CONIN - Console input
		SET_HIGH_REGISTER(AF, _getch());
#ifdef DEBUG
		if (HIGH_REGISTER(AF) == 4)
			Debug = 1;
#endif
		break;
	case 0x0C:				// 4 - CONOUT - Console output
		_putcon(LOW_REGISTER(BC));
		break;
	case 0x0F:				// 5 - LIST - List output
		break;
	case 0x12:				// 6 - PUNCH/AUXOUT - Punch output
		break;
	case 0x15:				// 7 - READER - Reader input (0x1a = device not implemented)
		SET_HIGH_REGISTER(AF, 0x1a);
		break;
	case 0x18:				// 8 - HOME - Home disk head
		break;
	case 0x1B:				// 9 - SELDSK - Select disk drive
		HL = 0x0000;
		break;
	case 0x1E:				// 10 - SETTRK - Set track number
		break;
	case 0x21:				// 11 - SETSEC - Set sector number
		break;
	case 0x24:				// 12 - SETDMA - Set DMA address
		HL = BC;
		dmaAddr = BC;
		break;
	case 0x27:				// 13 - READ - Read selected sector
		SET_HIGH_REGISTER(AF, 0x00);
		break;
	case 0x2A:				// 14 - WRITE - Write selected sector
		SET_HIGH_REGISTER(AF, 0x00);
		break;
	case 0x2D:				// 15 - LISTST - Get list device status
		SET_HIGH_REGISTER(AF, 0x0ff);
		break;
	case 0x30:				// 16 - SECTRAN - Sector translate
		HL = BC;			// HL=BC=No translation (1:1)
		break;
	default:
#ifdef DEBUG	// Show unimplementes calls only when debugging
		_puts("\r\nUnimplemented BIOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
#endif
		break;
	}
#ifdef DEBUGLOG
	_logBiosOut(ch);
#endif

}

void _Bdos(void)
{
	CPM_FCB* F;
	int32	i, c, count;
	uint8	ch = LOW_REGISTER(BC);

#ifdef DEBUGLOG
	_logBdosIn(ch);
#endif

	HL = 0x00;	// HL is reset by the BDOS
	SET_LOW_REGISTER(BC, LOW_REGISTER(DE)); // C ends up equal to E

	switch (ch) {
		/*
		C = 0 : System reset
		Doesn't return. Reloads CP/M
		*/
	case 0:
		Status = 2;	// Same as call to "BOOT"
		break;
		/*
		C = 1 : Console input
		Gets a char from the console
		Returns: A=Char
		*/
	case 1:
		HL = _getche();
#ifdef DEBUG
		if (HL == 4)
			Debug = 1;
#endif
		break;
		/*
		C = 2 : Console output
		E = Char
		Sends the char in E to the console
		*/
	case 2:
		_putcon(LOW_REGISTER(DE));
		break;
		/*
		C = 3 : Auxiliary (Reader) input
		Returns: A=Char
		*/
	case 3:
		HL = 0x1a;
		break;
		/*
		C = 4 : Auxiliary (Punch) output
		*/
	case 4:
		break;
		/*
		C = 5 : Printer output
		*/
	case 5:
		break;
		/*
		C = 6 : Direct console IO
		E = 0xFF : Checks for char available and returns it, or 0x00 if none (read)
		E = char : Outputs char (write)
		Returns: A=Char or 0x00 (on read)
		*/
	case 6:
		if (LOW_REGISTER(DE) == 0xff) {
			HL = _getchNB();
#ifdef DEBUG
			if (HL == 4)
				Debug = 1;
#endif
		} else {
			_putcon(LOW_REGISTER(DE));
		}
		break;
		/*
		C = 7 : Get IOBYTE
		Gets the system IOBYTE
		Returns: A = IOBYTE
		*/
	case 7:
		HL = _RamRead(0x0003);
		break;
		/*
		C = 8 : Set IOBYTE
		E = IOBYTE
		Sets the system IOBYTE to E
		*/
	case 8:
		_RamWrite(0x0003, LOW_REGISTER(DE));
		break;
		/*
		C = 9 : Output string
		DE = Address of string
		Sends the $ terminated string pointed by (DE) to the screen
		*/
	case 9:
		while ((ch = _RamRead(DE++)) != '$')
			_putcon(ch);
		break;
		/*
		C = 10 (0Ah) : Buffered input
		DE = Address of buffer
		Reads (DE) bytes from the console
		Returns: A = Number os chars read
		DE) = First char
		*/
	case 10:
		i = DE;
		c = _RamRead(i);	// Gets the number of characters to read
		i++;	// Points to the number read
		count = 0;
		while (c)	// Very simplistic line input (Lacks ^E ^R ^U support)
		{
			ch = _getch();
#ifdef DEBUG
			if (ch == 4)
				Debug = 1;
#endif
			if (ch == 3 && count == 0) {
				Status = 2;
				break;
			}
			if (ch == 0x0D || ch == 0x0A)
				break;
			if ((ch == 0x08 || ch == 0x7F) && count > 0) {
				_putcon('\b');
				_putcon(' ');
				_putcon('\b');
				count--;
				continue;
			}
			if (ch < 0x20 || ch > 0x7E)
				continue;
			_putcon(ch);
			count++; _RamWrite(i + count, ch);
			if (count == c)
				break;
		}
		_RamWrite(i, count);	// Saves the number or characters read
		break;
		/*
		C = 11 (0Bh) : Get console status
		Returns: A=0x00 or 0xFF
		*/
	case 11:
		HL = _chready();
		break;
		/*
		C = 12 (0Ch) : Get version number
		Returns: B=H=system type, A=L=version number
		*/
	case 12:
		HL = 0x22;
		break;
		/*
		C = 13 (0Dh) : Reset disk system
		*/
	case 13:
		roVector = 0;	// Make all drives R/W
		loginVector = 0;
		dmaAddr = 0x0080;
		_RamWrite(0x0004, 0x00);	// Reset default drive to A: and CP/M user to 0 (0x00)
		HL = _CheckSUB();			// Checks if there's a $$$.SUB on the boot disk
		break;
		/*
		C = 14 (0Eh) : Select Disk
		Returns: A=0x00 or 0xFF
		*/
	case 14:
		if (_SelectDisk(LOW_REGISTER(DE) + 1)) {
			_RamWrite(0x0004, (_RamRead(0x0004) & 0xf0) | (LOW_REGISTER(DE) & 0x0f));
			LastSel = _RamRead(0x0004);
		} else {
			_error(errSELECT);
			_RamWrite(0x0004, LastSel);	// Sets 0x0004 back to the last selected user/drive
		}
		break;
		/*
		C = 15 (0Fh) : Open file
		Returns: A=0x00 or 0xFF
		*/
	case 15:
		HL = _OpenFile(DE);
		break;
		/*
		C = 16 (10h) : Close file
		*/
	case 16:
		HL = _CloseFile(DE);
		break;
		/*
		C = 17 (11h) : Search for first
		*/
	case 17:
		HL = _SearchFirst(DE, TRUE);	// TRUE = Creates a fake dir entry when finding the file
		break;
		/*
		C = 18 (12h) : Search for next
		*/
	case 18:
		HL = _SearchNext(TRUE);			// TRUE = Creates a fake dir entry when finding the file
		break;
		/*
		C = 19 (13h) : Delete file
		*/
	case 19:
		HL = _DeleteFile(DE);
		break;
		/*
		C = 20 (14h) : Read sequential
		*/
	case 20:
		HL = _ReadSeq(DE);
		break;
		/*
		C = 21 (15h) : Write sequential
		*/
	case 21:
		HL = _WriteSeq(DE);
		break;
		/*
		C = 22 (16h) : Make file
		*/
	case 22:
		HL = _MakeFile(DE);
		break;
		/*
		C = 23 (17h) : Rename file
		*/
	case 23:
		HL = _RenameFile(DE);
		break;
		/*
		C = 24 (18h) : Return log-in vector (active drive map)
		*/
	case 24:
		HL = loginVector;	// (todo) improve this
		break;
		/*
		C = 25 (19h) : Return current disk
		*/
	case 25:
		HL = _RamRead(0x0004) & 0x0f;
		break;
		/*
		C = 26 (1Ah) : Set DMA address
		*/
	case 26:
		dmaAddr = DE;
		break;
		/*
		C = 27 (1Bh) : Get ADDR(Alloc)
		*/
	case 27:
		HL = SCBaddr;
		break;
		/*
		C = 28 (1Ch) : Write protect current disk
		*/
	case 28:
		roVector = roVector | (1 << (_RamRead(0x0004) & 0x0f));
		break;
		/*
		C = 29 (1Dh) : Get R/O vector
		*/
	case 29:
		HL = roVector;
		break;
		/********** (todo) Function 30: Set file attributes **********/
		/*
		C = 31 (1Fh) : Get ADDR(Disk Parms)
		*/
	case 31:
		HL = DPBaddr;
		break;
		/*
		C = 32 (20h) : Get/Set user code
		*/
	case 32:
		if (LOW_REGISTER(DE) == 0xFF) {
			HL = user;
		} else {
			user = LOW_REGISTER(DE);
			_RamWrite(0x0004, (_RamRead(0x0004) & 0x0f) | (LOW_REGISTER(DE) << 4));
		}
		break;
		/*
		C = 33 (21h) : Read random
		*/
	case 33:
		HL = _ReadRand(DE);
		break;
		/*
		C = 34 (22h) : Write random
		*/
	case 34:
		HL = _WriteRand(DE);
		break;
		/*
		C = 35 (23h) : Compute file size
		*/
	case 35:
		F = (CPM_FCB*)&RAM[DE];
		count = _FileSize(DE) >> 7;

		if (count == -1) {
			HL = 0xff;
		} else {
			F->r0 = count & 0xff;
			F->r1 = (count >> 8) & 0xff;
			F->r2 = (count >> 16) & 0xff;
		}
		break;
		/*
		C = 36 (24h) : Set random record
		*/
	case 36:
		F = (CPM_FCB*)&RAM[DE];
		count = F->cr & 0x7f;
		count += (F->ex & 0x1f) << 7;
		count += F->s1 << 12;
		count += F->s2 << 20;

		F->r0 = count & 0xff;
		F->r1 = (count >> 8) & 0xff;
		F->r2 = (count >> 16) & 0xff;
		break;
		/*
		C = 37 (25h) : Reset drive
		*/
	case 37:
		break;
		/********** Function 38: Not supported by CP/M 2.2 **********/
		/********** Function 39: Not supported by CP/M 2.2 **********/
		/********** (todo) Function 40: Write random with zero fill **********/
		/*
		C = 40 (28h) : Write random with zero fill (we have no disk blocks, so just write random)
		*/
	case 40:
		HL = _WriteRand(DE);
		break;
#ifdef ARDUINO
		/*
		C = 220 (DCh) : PinMode
		*/
	case 220:
		pinMode(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
		/*
		C = 221 (DDh) : DigitalRead
		*/
	case 221:
		HL = digitalRead(HIGH_REGISTER(DE));
		break;
		/*
		C = 222 (DEh) : DigitalWrite
		*/
	case 222:
		digitalWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
		/*
		C = 223 (DFh) : AnalogRead
		*/
	case 223:
		HL = analogRead(HIGH_REGISTER(DE));
		break;
		/*
		C = 224 (E0h) : AnalogWrite
		*/
	case 224:
		analogWrite(HIGH_REGISTER(DE), LOW_REGISTER(DE));
		break;
#endif
		/*
		Unimplemented calls get listed
		*/
	default:
#ifdef DEBUG	// Show unimplementes calls only when debugging
		_puts("\r\nUnimplemented BDOS call.\r\n");
		_puts("C = 0x");
		_puthex8(ch);
		_puts("\r\n");
#endif
		break;
	}

	// CP/M BDOS does this before returning
	SET_HIGH_REGISTER(BC, HIGH_REGISTER(HL));
	SET_HIGH_REGISTER(AF, LOW_REGISTER(HL));

#ifdef DEBUGLOG
	_logBdosOut(ch);
#endif

}
