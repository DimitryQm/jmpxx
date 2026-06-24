endbr64
testl	%edi, %edi
jne	.L6
xorl	%edx, %edx
movl	$9, %eax
salq	$32, %rdx
orq	%rdx, %rax
ret
.L6:
leal	5(%rdi,%rdi,4), %eax
movl	$1, %edx
addl	%eax, %eax
salq	$32, %rdx
movl	%eax, %eax
orq	%rdx, %rax
ret
