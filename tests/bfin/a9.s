//  ALU test program.
//  Test 32 bit MAX, MIN, ABS instructions
# mach: bfin

.include "testutils.inc"
	start


// MAX
// first operand is larger, so AN=0
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

// second operand is larger, so AN=1
	R0.L = 0x0000;
	R0.H = 0x0000;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

// first operand is larger, check correct output with overflow
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );

// second operand is larger, no overflow here
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0xffff;
	R1.H = 0x7fff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );

// second operand is larger, overflow
	R0.L = 0xffff;
	R0.H = 0x800f;
	R1.L = 0xffff;
	R1.H = 0x7fff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );

// both operands equal
	R0.L = 0x0080;
	R0.H = 0x8000;
	R1.L = 0x0080;
	R1.H = 0x8000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0080 );
	DBGA ( R7.H , 0x8000 );

// MIN
// second operand is smaller
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );

// first operand is smaller
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );

// first operand is smaller, overflow
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0000;
	R1.H = 0x0ff0;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );

// equal operands
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0001;
	R1.H = 0x8000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );

// ABS
	R0.L = 0x0001;
	R0.H = 0x8000;
	R7 = ABS R0;
	_DBG R7;
	_DBG ASTAT;

	R0.L = 0x0001;
	R0.H = 0x0000;
	R7 = ABS R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R7 = ABS R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );

	R0.L = 0x0000;
	R0.H = 0x0000;
	R7 = ABS R0;
	_DBG R7;

	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );

	pass

