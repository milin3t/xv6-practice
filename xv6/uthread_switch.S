	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	pushal

	movl current_thread, %eax	
   	movl %esp, (%eax)	

	movl next_thread, %eax		
	movl (%eax), %esp		

	movl %eax, current_thread
	movl $0 , next_thread
	
	popal

	ret				/* pop return address from stack */
