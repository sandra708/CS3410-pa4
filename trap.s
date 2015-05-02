
.bss

.global trap_save_state
trap_save_state:
/* struct mips_core_data */
.word 0 /* r0 */
.word 0 /* r1 */
.word 0 /* r2 */
.word 0 /* r3 */
.word 0 /* r4 */
.word 0 /* r5 */
.word 0 /* r6 */
.word 0 /* r7 */
.word 0 /* r8 */
.word 0 /* r9 */
.word 0 /* r10 */
.word 0 /* r11 */
.word 0 /* r12 */
.word 0 /* r13 */
.word 0 /* r14 */
.word 0 /* r15 */
.word 0 /* r16 */
.word 0 /* r17 */
.word 0 /* r18 */
.word 0 /* r19 */
.word 0 /* r20 */
.word 0 /* r21 */
.word 0 /* r22 */
.word 0 /* r23 */
.word 0 /* r24 */
.word 0 /* r25 */
.word 0 /* r26 */
.word 0 /* r27 */
.word 0 /* r28 */
.word 0 /* r29 */
.word 0 /* r30 */
.word 0 /* r31 */
.word 0 /* LO */
.word 0 /* HI */
.word 0 /* PC */

.section .traphandler /* linker will put this code at the appropriate place in memory */

/* extern void trap_handler(struct mips_core_data *save_state, unsigned int status, unsigned int cause) */
.extern trap_handler 

/* extern void *trap_stack_top; */
.extern trap_stack_top
.extern trap_gp

.func	trap
.type	trap, @function
trap:
  .set noat /* tell compiler not to use $at register */
  /* get the current cpu ID */
  mfc0 $27, $16
  sll  $27, $27, 2	  /* r27 = 4*ID */

  /* figure out where to save state, namely, trap_save_state[id] */
  la $26, trap_save_state /* r26 = pointer to [array of pointers to save-state structs] */
  add $26, $26, $27	  /* r26 = pointer to i-th element of array */
  lw $26, 0($26)	  /* r26 = pointer to save-state struct  */

  /* save all state */
                   /* r0 */
  sw $1,    4($26) /* r1 */
  .set at /* tell compiler it can use $at register */
  sw $2,    8($26) /* r2 */
  sw $3,   12($26) /* r3 */
  sw $4,   16($26) /* r4 */
  sw $5,   20($26) /* r5 */
  sw $6,   24($26) /* r6 */
  sw $7,   28($26) /* r7 */
  sw $8,   32($26) /* r8 */
  sw $9,   36($26) /* r9 */
  sw $10,  40($26) /* r10 */
  sw $11,  44($26) /* r11 */
  sw $12,  48($26) /* r12 */
  sw $13,  52($26) /* r13 */
  sw $14,  56($26) /* r14 */
  sw $15,  60($26) /* r15 */
  sw $16,  64($26) /* r16 */
  sw $17,  68($26) /* r17 */
  sw $18,  72($26) /* r18 */
  sw $19,  76($26) /* r19 */
  sw $20,  80($26) /* r20 */
  sw $21,  84($26) /* r21 */
  sw $22,  88($26) /* r22 */
  sw $23,  92($26) /* r23 */
  sw $24,  96($26) /* r24 */
  sw $25, 100($26) /* r25 */
                   /* r26 ($k0) */
                   /* r27 ($k1) */
  sw $28, 112($26) /* r28 */
  sw $29, 116($26) /* r29 */
  sw $30, 120($26) /* r30 */
  sw $31, 124($26) /* r31 */
  mflo $8
  mfhi $9
  mfc0 $10, $15
  sw $8,  128($26) /* LO */
  sw $9,  132($26) /* HI */
  sw $10, 136($26) /* PC (actually, EPC) */

  /* set up kernel stack and global pointer */
  la $10, trap_stack_top /* r10 = pointer to [array of stack addresses] */
  add $10, $27		 /* r10 = pointer to i-th element of array */
  lw $sp, 0($10)	 /* sp = i-th element of array */

  la $10, trap_gp	 /* r10 = pointer to [array of gp addresses] */
  add $10, $27		 /* r10 = pointer to i-th element of array */
  lw $gp, 0($10)	 /* gp = i-th element of array */

  addi $sp, $sp, -4
  move $fp, $sp

  /* keep the save_state pointer in caller-save register s0 */
  move $16, $26

  /* call trap handler (written in C) */
  move $4, $26 /* a0 = pointer to save-state struct */
  mfc0 $5, $13 /* a1 = status */
  mfc0 $6, $14 /* a2 = cause */
  jal trap_handler  

  /* restore state from trap_save_state */
  move $26, $16

  lw $8,  128($26) /* LO */
  lw $9,  132($26) /* HI */
  lw $10, 136($26) /* PC (actually, EPC) */
  mtlo $8
  mthi $9
  mtc0 $10, $15

		   /* r0 */
  lw $27,   4($26) /* r1 */
  lw $2,    8($26) /* r2 */
  lw $3,   12($26) /* r3 */
  lw $4,   16($26) /* r4 */
  lw $5,   20($26) /* r5 */
  lw $6,   24($26) /* r6 */
  lw $7,   28($26) /* r7 */
  lw $8,   32($26) /* r8 */
  lw $9,   36($26) /* r9 */
  lw $10,  40($26) /* r10 */
  lw $11,  44($26) /* r11 */
  lw $12,  48($26) /* r12 */
  lw $13,  52($26) /* r13 */
  lw $14,  56($26) /* r14 */
  lw $15,  60($26) /* r15 */
  lw $16,  64($26) /* r16 */
  lw $17,  68($26) /* r17 */
  lw $18,  72($26) /* r18 */
  lw $19,  76($26) /* r19 */
  lw $20,  80($26) /* r20 */
  lw $21,  84($26) /* r21 */
  lw $22,  88($26) /* r22 */
  lw $23,  92($26) /* r23 */
  lw $24,  96($26) /* r24 */
  lw $25, 100($26) /* r25 */
 		   /* r26 ($k0) */
                   /* r27 ($k1) */
  lw $28, 112($26) /* r28 */
  lw $29, 116($26) /* r29 */
  lw $30, 120($26) /* r30 */
  lw $31, 124($26) /* r31 */

  /* restore $at last */
  .set noat  /* tell compiler not to use $at register */
  move $at, $27

  /* jump to EPC and restore interrupt enable to whatever it was before the trap */
  .set mips3 /* tell compiler it is okay to use mips3 ERET instruction */
  eret
  .set mips1 /* tell compiler to use only mips1 instructions */

  .set at    /* tell compiler it can use $at register */
.endfunc

