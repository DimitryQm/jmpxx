endbr64
movl	%edi, %eax
btsq	$32, %rax
ret
addq	$1, %rax
