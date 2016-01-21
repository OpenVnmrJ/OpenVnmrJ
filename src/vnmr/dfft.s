#ifdef sun3
| 
|
|********************************************************
|							*
|	dfft: fast 32 bit integer fft for 68020		*
|							*
|********************************************************
|
|  Rene Richarz, October 86
|
|  Needs complex double precision sine table
|
|  Operation:
|    Perform integer fft, scale down 2 bits whenever 
|         a overflow occurs.
|  
|  Note: the algorithm used in this fft requires bit
|	 reversed input data. 
|
|  call from c:
|    dfft(data, poweroftwo, &scale,sinetable,size)
|
	.globl	_dfft
|
	temp	= d0		| Intermediate register
	ix	= d1		| Intermediate result in butterfly
	iy	= d2		| Intermediate result in butterfly
	butcnt	= d3		| counts butterflies in block
	offset	= d4		| offset between A and B in butterfly
	woffset	= d5		| offset for sine table
	t2	= d6
	stgcnt	= d7		| counts stages
|
	apnt	= a0		| Pointer to point A in butterfly
	bpnt	= a1		| Pointer to point B in butterfly
	wpnt	= a2		| Pointer to sine/cosine in butterfly
	vartb	= a3		| Pointer to variable table
	psp	= a4		| Parameter stack pointer
|
| vartb: table with offsets of variables stored on the stack
|
	n	= 0		| (long) No of complex points
	data	= 4		| (long) pointer to start of data table
	scf	= 8		| (long) scaling factor
	blinst	= 12		| (long) blocks in stage
	buinbl	= 16		| (long) butterflies in block
	sintb	= 20		| (long) pointer to current sine table
	scpnt	= 24		| (long) pointer to scaling factor
	blkcnt  = 28
	tabsize = 32
|
|
_dfft:  link	psp,#0
	moveml	d0-d7/a0-a4,sp@-	| save registers
        subl	#tabsize,sp
	movl	sp,vartb
|
	clrl	temp
        movel	psp@(0x18),woffset   	| woffset := first offset in sinetable
	movel	psp@(0x14),vartb@(sintb)| pointer to start of sine table
	movel	psp@(0x10),vartb@(scpnt)| scpnt := pointer to scaling f.
|					==============================
	movel	psp@(0xc),ix		| ix := poweroftwo
	movel	ix,stgcnt		| stgcnt := poweroftwo
|					====================
	movel	psp@(0x8),vartb@(data)	| data := pointer to data
|					=======================
|
	movel	#1,temp			| temp := 1
	subqw	#1,ix			| correct for loop
dfft1:
	asll	#1,temp			| compute no of points in temp
	dbf	ix,dfft1
	movel	temp,vartb@(n)		| n := no of complex points
|					=========================
	lsrl	#1,temp
	movel	temp,vartb@(blinst)	| blinst := n / 2
|					===============
	movel	#0,vartb@(scf)		| scf := 0
|					========
	movel	#1,vartb@(buinbl)	| buinbl := 1
|					===========
	movel	#8,offset		| offset := 8 bytes
|					=================
	subqw	#1,stgcnt		| correct for loop
|
dfft2:	
|
|********************************************************
|							*
|	Stage: Does one stage for 32-bit fft		*
|							*
|********************************************************
|
|	offset:		butterfly offset in this stage
|	blinst:		number of blocks in current stage
|	buinbl:		number of butterflies in block
|	sintb:		currents start of sine table
|	woffset:	current sine table offset
|
stage:
	movel	vartb@(data),bpnt	| first block address
	movel	vartb@(blinst),vartb@(blkcnt)
|
stage1:
|
|
|********************************************************
|							*
|	Block: Does one block within a stage		*
|							*
|********************************************************
|
| 	Input:
|	  bpnt:		start of data block
|	  offset:	butterfly offset in bytes
|	  buinbl:	no of butterflies per block
|	  sintb:	pointer to first sine/cosine
|	  woffset:	offset in sine/cosine table in bytes
|
block:
	movel	bpnt,apnt
	addl	offset,bpnt		| bpnt := apnt + offset
	movel	vartb@(buinbl),butcnt
	movel	vartb@(sintb),wpnt	| 	start with zero element
|
block1:
|
|********************************************************
|							*
|	Butterfly for 32 bit integer fft		*
|							*
|********************************************************
|
|  Input are complex points A and B
|  Output are complex ponts A' and B'
|
|    A' := A + Wnr B
|    B' := A - Wnr B
|  temporary register:
|
butfly:
|
	movel	wpnt@(4),t2	| t2 := Wy
        mulsl   bpnt@(4),temp:t2
        asll	#1,temp		| temp := Wy * By
	movel	wpnt@,t2	| t2 := Wx
        mulsl   bpnt@,ix:t2
        asll	#1,ix		| ix := Wx * Bx
	subl	temp,ix		| ix := Wx * Bx - Wy * By
|				=======================
	bvc	butf1		| skip, if no overflow
	addl	temp,ix		| restore original value
	asrl	#2,temp		| scale intermediate results
	asrl	#2,ix
	subl	temp,ix		| compute ix again
	bsr	scale		| and scale whole data
|
butf1:  movel	wpnt@(4),t2	| t2 := Wy
        mulsl   bpnt@,temp:t2
        asll	#1,temp		| temp := Wy * Bx
	movel	wpnt@,t2	| t2 := Wx
        mulsl   bpnt@(4),iy:t2
        asll	#1,iy		| iy := Wx * By
	addl	temp,iy		| iy := Wx * By + Wy * Bx
|				=======================
	bvc	butf2		| skip, if no overflow
	subl	temp,iy		| restore original value
	asrl	#2,temp		| scale intermediate results
	asrl	#2,iy
	asrl	#2,ix
	addl	temp,iy		| compute iy again	
	bsr	scale		| and scale whole data
|
butf2:
	addl	ix,apnt@	| ax := ax + ix
|				=============
	bvc	butf3		| skip, if no overflow
	subl	ix,apnt@	| restore original value
	asrl	#2,ix		| scale intermediate results
	asrl	#2,iy
	bsr	scale		| and scale whole data
	addl	ix,apnt@	| and repeat calculation
|
butf3:
	addl	iy,apnt@(4)	| ay := ay + iy
|				=============
	bvc	butf4		| skip, if no overflow
	subl	iy,apnt@(4)	| restore original value
	asrl	#2,ix		| scale intermediate results
	asrl	#2,iy
	bsr	scale		| and whole data
	addl	iy,apnt@(4)	| and repeat calculation
|
butf4:
	movel	apnt@,temp	| temp := ax + ix
	subl	ix,temp		| temp := ax
	subl	ix,temp		| temp := ax - ix
	bvc	butf5		| skip, if no overflow
	addl	ix,temp		| restore original value
	asrl	#2,ix		| scale intermediate results
	asrl	#2,iy
	asrl	#2,temp
	bsr	scale		| and whole data
	subl	ix,temp		| and repeat calculation
|
butf5:
	movel	temp,bpnt@	| bx := ax - ix
|				=============
	movel	apnt@(4),temp	| temp := ay + iy
	subl	iy,temp
	subl	iy,temp		| temp := ay - iy
	bvc	butf6		| skip, if no overflo
	addl	iy,temp		| restore original value
	asrl	#2,iy		| scale intermediate results
	asrl	#2,temp
	bsr	scale		| and whole data
	subl	iy,temp		| and repeat calculation
|
butf6:
	movel	temp,bpnt@(4)	| by := ay - iy
|				=============
|********************************************************
|
|	end of butterfly - finish up block
|
|********************************************************
	addql	#8,apnt			| compute new pointer to A
	addql	#8,bpnt			| compute new pointer to B
	addl	woffset,wpnt		| compute new pointer to W
	subql	#1,butcnt
	bne	block1			| loop until done
|*******************************************************
|
|	end of block - finish up stage
|
|*******************************************************
	subql	#1,vartb@(blkcnt)
	bne	stage1			| loop until done
|
	asll	#1,offset		| next stage has twice offset
        movel	vartb@(blinst),temp
	lsrl	#1,temp
        movel	temp,vartb@(blinst)	| next stage has half no of blocks
	movel	vartb@(buinbl),temp
	asll	#1,temp
	movel	temp,vartb@(buinbl)	| next stage has twice but per blk
	lsrl	#1,woffset		| next stage has half sine offset
 	dbf	stgcnt,dfft2		| loop until done
|
|
	movel	vartb@(scpnt),apnt
	movel	vartb@(scf),apnt@	| 	give scaling factor back
|
        addl	#tabsize,sp
	moveml 	sp@+,d0-d7/a0-a4	| recall saved registers
	unlk	psp
	rts
|
|
|********************************************************
|							*
|	Scale: Scaling routine for 32 bit integer fft	*
|							*
|********************************************************
|
|	Divides whole data table by two
|	Data table starts at data (pointer in vartb)
|	Size of data is n (pointer in vartb) complex points
|
	stemp	= d0			| used, but preserved
	sh	= d1
	spnt	= a0			| used, but preserved
|
scale:
	moveml	d0-d1/a0,sp@-		| save registers on stack
|
	movel	vartb@(n),stemp		| stemp := number of complex points
	movel	vartb@(data),spnt	| points to start of data table
|
scale1:
	movel	spnt@,sh
	asrl	#2,sh
	movel	sh,spnt@+
	movel	spnt@,sh
	asrl	#2,sh
	movel	sh,spnt@+
	subql	#1,stemp		| correct loop counter
	bne	scale1			| loop until done
|
	addql	#2,vartb@(scf)	| scf := scf + 1;
|
	moveml	sp@+,d0-d1/a0
	rts
#endif
