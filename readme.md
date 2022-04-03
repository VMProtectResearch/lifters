vtil or llvm lifters


目前已知的几个问题:

1.llvm的优化把inline asm的调用位置给改了,这会不会有问题?

2.单个lift编译成x86测试的没问题,都搞一块的话llvm编译的x86代码也会出问题
