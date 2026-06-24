endbr64
testl	%edi, %edi
leal	7(%rdi,%rdi), %eax
movl	$-1, %edx
cmovs	%edx, %eax
ret
