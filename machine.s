
.text

/* misc. helpers */

.global current_cpu_gp
.ent	current_cpu_gp
.type	current_cpu_gp, @function
current_cpu_gp:
  move $2, $28
  jr $ra
.end	current_cpu_gp


/* read various coprocessor registers */

.global current_cpu_context
.ent	current_cpu_context
.type	current_cpu_context, @function
current_cpu_context:
  mfc0 $2, $4
  jr $ra
.end	current_cpu_context

.global current_cpu_status
.ent	current_cpu_status
.type	current_cpu_status, @function
current_cpu_status:
  mfc0 $2, $13
  jr $ra
.end	current_cpu_status

.global current_cpu_cause
.ent	current_cpu_cause
.type	current_cpu_cause, @function
current_cpu_cause:
  mfc0 $2, $14
  jr $ra
.end	current_cpu_cause

.global current_cpu_epc
.ent	current_cpu_epc
.type	current_cpu_epc, @function
current_cpu_epc:
  mfc0 $2, $15
  jr $ra
.end	current_cpu_epc

.global current_cpu_badvaddr
.ent	current_cpu_badvaddr
.type	current_cpu_badvaddr, @function
current_cpu_badvaddr:
  mfc0 $2, $8
  jr $ra
.end	current_cpu_badvaddr

.global current_cpu_cycles
.ent	current_cpu_cycles
.type	current_cpu_cycles, @function
current_cpu_cycles:
  mfc0 $2, $9
  jr $ra
.end	current_cpu_cycles

.global current_cpu_id
.ent	current_cpu_id
.type	current_cpu_id, @function
current_cpu_id:
  mfc2 $2, $16
  jr $ra
.end	current_cpu_id

.global current_cpu_enable
.ent	current_cpu_enable
.type	current_cpu_enable, @function
current_cpu_enable:
  mfc2 $2, $17
  jr $ra
.end	current_cpu_enable

.global current_cpu_exists
.ent	current_cpu_exists
.type	current_cpu_exists, @function
current_cpu_exists:
  mfc2 $2, $18
  jr $ra
.end	current_cpu_exists

/* write various coprocessor registers */

.global set_cpu_context
.ent	set_cpu_context
.type	set_cpu_context, @function
set_cpu_context:
  mtc0 $4, $4
  jr $ra
.end	set_cpu_context

.global set_cpu_status
.ent	set_cpu_status
.type	set_cpu_status, @function
set_cpu_status:
  mtc0 $4, $13
  jr $ra
.end	set_cpu_status

.global set_cpu_cause
.ent	set_cpu_cause
.type	set_cpu_cause, @function
set_cpu_cause:
  mtc0 $4, $14
  jr $ra
.end	set_cpu_cause

.global set_cpu_epc
.ent	set_cpu_epc
.type	set_cpu_epc, @function
set_cpu_epc:
  mtc0 $4, $15
  jr $ra
.end	set_cpu_epc

.global set_cpu_badvaddr
.ent	set_cpu_badvaddr
.type	set_cpu_badvaddr, @function
set_cpu_badvaddr:
  mtc0 $4, $8
  jr $ra
.end	set_cpu_badvaddr

.global set_cpu_enable
.ent	set_cpu_enable
.type	set_cpu_enable, @function
set_cpu_enable:
  mtc2 $4, $17
  jr $ra
.end	set_cpu_enable
