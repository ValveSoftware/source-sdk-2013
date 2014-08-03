//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef P5P6PERFORMANCECOUNTERS_H
#define P5P6PERFORMANCECOUNTERS_H

// defined for < Pentium 4

//---------------------------------------------------------------------------
// Format of the performance event IDs within this header file in case you
// wish to add any additional events that may not be present here.
//
// BITS 0-8	Unit Mask, Unsed on P5 processors
// BIT  9	Set if event can be set on counter 0
// BIT  10	Set if event can be set on counter 1
// BITS 11-15	Unused Set to zero
// BITS 16-23	Event Select ID, Only bits 16-21 used on P5 Family
// BITS 24-27	Unused set to zero
// BITS 28-32	Process family that the event belong to, as returned by
//		the CPUID instruction.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// PENTIUM PERFORMANCE COUNTERS.
//---------------------------------------------------------------------------
#define	P5_DTCRD 0x50000300 //Data Cache Reads
#define	P5_DWRIT 0x50010300 //Data Cache Writes
#define	P5_DTTLB 0x50020300 //Data TLB Miss
#define	P5_DTRMS 0x50030300 //Data Read Misses
#define	P5_DWRMS 0x50040300 //Data Write Miss
#define	P5_WHLCL 0x50050300 //Write (Hit) to M- or E-state line
#define	P5_DCLWB 0x50060300 //Data Cache Lines Written Back
#define	P5_DCSNP 0x50070300 //External Snoops
#define	P5_DCSHT 0x50080300 //Data Cache Snoop Hits
#define	P5_MAIBP 0x50090300 //Memory Access in Both Pipes
#define	P5_BANKS 0x500A0300 //Bank Conflicts
#define	P5_MISAL 0x500B0300 //Misaligned Data Memory Reference
#define	P5_COCRD 0x500C0300 //Code Cache Reads
#define	P5_COTLB 0x500D0300 //Code TLB Misses
#define	P5_COCMS 0x500E0300 //Code Cache Misses
#define	P5_ANYSG 0x500F0300 //Any Segment Register Loaded
#define	P5_BRANC 0x50120300 //Branches
#define	P5_BTBHT 0x50130300 //BTB Hits
#define	P5_TBRAN 0x50140300 //Taken Branch or BTB hit
#define	P5_PFLSH 0x50150300 //Pipeline flushes
#define	P5_INSTR 0x50160300 //Instructions Executed
#define	P5_INSTV 0x50170300 //Instructions Executed in the V-Pipe (Pairing)
#define	P5_CLOCL 0x50180300 //Bus active
#define	P5_PSDWR 0x50190300 //Full write buffers
#define	P5_PSWDR 0x501A0300 //Waiting for Data Memory Read
#define	P5_NCLSW 0x501B0300 //Clocks stalled writing an E or M state line
#define	P5_IORWC 0x501D0300 //I/O Read or Write Cycle
#define	P5_NOCMR 0x501E0300 //Non-Cacheable Memory Reads
#define	P5_PSLDA 0x501F0300 //Clocks stalled due to AGI
#define	P5_FLOPS 0x50220300 //Floating Point Operations
#define P5_DBGR0 0x50230300 //Breakpoint match on DR0
#define P5_DBGR1 0x50240300 //Breakpoint match on DR1
#define P5_DBGR2 0x50250300 //Breakpoint match on DR2
#define P5_DBGR3 0x50260300 //Breakpoint match on DR3
#define	P5_HWINT 0x50270300 //Hardware interrupts
#define	P5_DTRWR 0x50280300 //Data reads or writes
#define	P5_DTRWM 0x50290300 //Data read or write miss
#define P5_BOLAT 0x502A0100 //Bus ownership latency
#define P5_BOTFR 0x502A0200 //Bus ownership transfer
#define	P5_MMXA1 0x502B0100 //MMX Instruction Executed in U-pipe
#define	P5_MMXA2 0x502B0200 //MMX Instruction Executed in V-pipe
#define P5_MMXMS 0x502C0100 //Cache M state line sharing
#define P5_MMSLS 0x502C0200 //Cache line sharing
#define	P5_MMXB1 0x502D0100 //EMMS Instructions Executed
#define	P5_MMXB2 0x502D0200 //Transition from MMX to FP instructions
#define	P5_NOCMW 0x502E0200 //Non-Cacheable Memory Writes
#define	P5_MMXC1 0x502F0100 //Saturated MMX Instructions Executed
#define	P5_MMXC2 0x502F0200 //Saturations Performed
#define	P5_MMXHS 0x50300100 //Cycles Not in HALT State
#define	P5_MMXD2 0x50310100 //MMX Data Read
#define	P5_MMXFP 0x50320100 //Floating Point Stalls
#define	P5_MMXTB 0x50320200 //Taken Branches
#define	P5_MMXD0 0x50330100 //D1 Starvation and FIFO Empty
#define	P5_MMXD1 0x50330200 //D1 Starvation and one instruction in FIFO
#define	P5_MMXE1 0x50340100 //MMX Data Writes
#define	P5_MMXE2 0x50340200 //MMX Data Write Misses
#define	P5_MMXWB 0x50350100 //Pipeline flushes, wrong branch prediction
#define	P5_MMXWJ 0x50350200 //Pipeline flushes, branch prediction WB-stage
#define	P5_MMXF1 0x50360100 //Misaligned MMX Data Memory Reference
#define	P5_MMXF2 0x50360200 //Pipeline Stalled Waiting for MMX data read
#define	P5_MMXRI 0x50370100 //Returns Predicted Incorrectly
#define	P5_MMXRP 0x50370200 //Returns Predicted
#define	P5_MMXG1 0x50380100 //MMX Multiply Unit Interlock
#define	P5_MMXG2 0x50380200 //MOVD/MOVQ store stall, previous operation
#define	P5_MMXRT 0x50390100 //Returns
#define	P5_MMXRO 0x50390200 //RSB Overflows
#define	P5_MMXBF 0x503A0100 //BTB False entries
#define	P5_MMXBM 0x503A0200 //BTB misprediction on a Not-Taken Branch
#define	P5_PXDWR 0x503B0100 //stalled due MMX Full write buffers
#define	P5_PXZWR 0x503B0200 //stalled on MMX write to E or M state line

#define	P5_CLOCK 0x503F0300 //Special value to count clocks on P5


//---------------------------------------------------------------------------
// PENTIUM PRO / PENTIUM II  PERFORMANCE COUNTERS.
//---------------------------------------------------------------------------
#define	P6_STRBB 0x60030300 //Store Buffer Block
#define	P6_STBDC 0x60040300 //Store Buffer Drain Cycles
#define	P6_MISMM 0x60050300 //Misaligned Data Memory Reference
#define	P6_SEGLD 0x60060300 //Segment register loads
#define	P6_FPOPE 0x60100100 //FP Computational Op. (COUNTER 0 ONLY)
#define	P6_FPEOA 0x60110200 //FP Microcode Exceptions (COUNTER 1 ONLY)
#define	P6_FMULT 0x60120200 //Multiplies (COUNTER 1 ONLY)
#define	P6_FPDIV 0x60130200 //Divides (COUNTER 1 ONLY)
#define	P6_DBUSY 0x60140200 //Cycles Divider Busy  (COUNTER 1 ONLY)
#define	P6_L2STR 0x60210300 //L2 address strobes => address bus utilization
#define	P6_L2BBS 0x60220300 //Cycles L2 Bus Busy
#define	P6_L2BBT 0x60230300 //Cycles L2 Bus Busy transferring data to CPU
#define	P6_L2ALO 0x60240300 //L2 Lines Allocated
#define	P6_L2MAL 0x60250300 //L2 M-state Lines Allocated
#define	P6_L2CEV 0x60260300 //L2 Lines Evicted
#define	P6_L2MEV 0x60270300 //L2 M-state Lines Evicted
#define	P6_L2MCF 0x60280301 //L2 Cache Instruction Fetch Misses
#define	P6_L2FET 0x6028030F //L2 Cache Instruction Fetches
#define	P6_L2DRM 0x60290301 //L2 Cache Read Misses
#define	P6_L2DMR 0x6029030F //L2 Cache Reads
#define	P6_L2DWM 0x602A0301 //L2 Cache Write Misses
#define	P6_L2DMW 0x602A030F //L2 Cache Writes
#define	P6_L2CMS 0x602E0301 //L2 Cache Request Misses
#define	P6_L2DCR 0x602E030F //L2 Cache Requests
#define	P6_DMREF 0x60430300 //Data Memory References
#define	P6_DCALO 0x6045030F //L1 Lines Allocated
#define	P6_DCMAL 0x60460300 //L1 M-state Data Cache Lines Allocated
#define	P6_DCMEV 0x60470300 //L1 M-state Data Cache Lines Evicted
#define	P6_DCOUT 0x60480300 //L1 Misses outstanding
#define	P6_TSMCD 0x60520300 //Time Self-Modifiying Code Detected
#define	P6_BRWRA 0x60600300 //External Bus Cycles While Receive Active
#define	P6_BRDCD 0x60600300 //External Bus Request Outstanding
#define	P6_BRBNR 0x60610300 //External Bus Cycles While BNR Asserted
#define	P6_BUSBS 0x60620300 //External Bus Cycles-DRDY Asserted (busy)
#define	P6_BLOCK 0x60630300 //External Bus Cycles-LOCK signal asserted
#define	P6_BBRCV 0x60640300 //External Bus Cycles-Processor receiving data
#define	P6_BURST 0x60650300 //External Bus Burst Read Operations
#define	P6_BRINV 0x60660300 //External Bus Read for Ownership Transaction
#define	P6_BMLEV 0x60670300 //External Bus Writeback M-state Evicted
#define	P6_BBIFT 0x60680300 //External Bus Burst Instruction Fetches
#define	P6_BINVL 0x60690300 //External Bus Invalidate Transactions
#define	P6_BPRBT 0x606A0300 //External Bus Partial Read Transactions
#define	P6_BPTMO 0x606B0300 //External Bus Partial Memory Transactions
#define	P6_BUSIO 0x606C0300 //External Bus I/O Bus Transactions
#define	P6_BUSDF 0x606D0300 //External Bus Deferred Transactions
#define	P6_BUSTB 0x606E0300 //External Bus Burst Transactions
#define	P6_BMALL 0x606F0300 //External Bus Memory Transactions
#define	P6_BSALL 0x60700300 //External Bus Transactions
#define	P6_CLOCK 0x60790300 //Clockticks
#define	P6_BRHIT 0x607A0300 //External Bus Cycles While HIT Asserted
#define	P6_BRHTM 0x607B0300 //External Bus Cycles While HITM Asserted
#define	P6_BRSST 0x607E0300 //External Bus Cycles While Snoop Stalled
#define	P6_CMREF 0x60800300 //Total Instruction Fetches
#define	P6_TOIFM 0x60810300 //Total Instruction Fetch Misses
#define	P6_INTLB 0x60850300 //Instructions TLB Misses
#define	P6_CSFET 0x60860300 //Cycles Instruction Fetch Stalled
#define	P6_FTSTL 0x60870300 //Cycles Instruction Fetch stalled
#define	P6_RSTAL 0x60A20300 //Resource Related Stalls
#define	P6_MMXIE 0x60B00300 //MMX Instructions Executed
#define	P6_SAISE 0x60B10300 //Saturated Arithmetic Instructions Executed
#define	P6_PORT0 0x60B20301 //MMX micro-ops executed on Port 0
#define	P6_PORT1 0x60B20302 //MMX micro-ops executed on Port 1
#define	P6_PORT2 0x60B20304 //MMX micro-ops executed on Port 2
#define	P6_PORT3 0x60B20308 //MMX micro-ops executed on Port 3
#define	P6_MMXPA 0x60B30300 //MMX Packed Arithmetic
#define	P6_MMXPM 0x60B30301 //MMX Packed Multiply
#define	P6_MMXPS 0x60B30302 //MMX Packed Shift
#define	P6_MMXPO 0x60B30304 //MMX Packed Operations
#define	P6_MMXUO 0x60B30308 //MMX Unpacked Operations
#define	P6_MMXPL 0x60B30310 //MMX Packed Logical
#define	P6_INSTR 0x60C00300 //Instructions Retired
#define	P6_FPOPS 0x60C10100 //FP operations retired (COUNTER 0 ONLY)
#define	P6_UOPSR 0x60C20300 //Micro-Ops Retired
#define	P6_BRRET 0x60C40300 //Branch Instructions Retired
#define	P6_BRMSR 0x60C50300 //Branch Mispredictions Retired
#define	P6_MASKD 0x60C60300 //Clocks while interrupts masked
#define	P6_MSKPN 0x60C70300 //Clocks while interrupt is pending
#define	P6_HWINT 0x60C80300 //Hardware Interrupts Received
#define	P6_BTAKR 0x60C90300 //Taken Branch Retired
#define	P6_BTAKM 0x60CA0300 //Taken Branch Mispredictions
#define	P6_FPMMX 0x60CC0301 //Transitions from Floating Point to MMX
#define	P6_MMXFP 0x60CC0300 //Transitions from MMX to Floating Point
#define	P6_SIMDA 0x60CD0300 //SIMD Assists (EMMS Instructions Executed)
#define	P6_MMXIR 0x60CE0300 //MMX Instructions Retired
#define	P6_SAISR 0x60CF0300 //Saturated Arithmetic Instructions Retired
#define	P6_INSTD 0x60D00300 //Instructions Decoded
#define	P6_NPRTL 0x60D20300 //Renaming Stalls
#define	P6_SRSES 0x60D40301 //Segment Rename Stalls - ES
#define	P6_SRSDS 0x60D40302 //Segment Rename Stalls - DS
#define	P6_SRSFS 0x60D40304 //Segment Rename Stalls - FS
#define	P6_SRSGS 0x60D40308 //Segment Rename Stalls - GS
#define	P6_SRSXS 0x60D4030F //Segment Rename Stalls - ES DS FS GS
#define	P6_SRNES 0x60D50301 //Segment Renames - ES
#define	P6_SRNDS 0x60D50302 //Segment Renames - DS
#define	P6_SRNFS 0x60D50304 //Segment Renames - FS
#define	P6_SRNGS 0x60D50308 //Segment Renames - GS
#define	P6_SRNXS 0x60D5030F //Segment Renames - ES DS FS GS
#define	P6_BRDEC 0x60E00300 //Branch Instructions Decoded
#define	P6_BTBMS 0x60E20301 //BTB Misses
#define	P6_RETDC 0x60E40300 //Bogus Branches
#define	P6_BACLR 0x60E60300 //BACLEARS Asserted (Testing)






// INTEL
#define PENTIUM_FAMILY	  5	// define for pentium
#define PENTIUMPRO_FAMILY 6	// define for pentium pro
#define PENTIUM4_FAMILY   15	// define for pentium 4


// AMD
#define K6_FAMILY	       5	
#define K8_FAMILY           6	
#define EXTENDED_FAMILY   15  // AMD 64 and AMD Opteron

#endif // P5P6PERFORMANCECOUNTERS_H
