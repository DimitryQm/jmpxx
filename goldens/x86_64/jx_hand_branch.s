endbr64
testl	%edi, %edi
jne	.L8
xorl	%eax, %eax
movl	$9, %edx
orq	%rdx, %rax
ret
.L8:
movabsq	$4294967296, %rax
leal	5(%rdi,%rdi,4), %edx
addl	%edx, %edx
orq	%rdx, %rax
ret
