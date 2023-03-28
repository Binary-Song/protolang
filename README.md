打包

```
git archive --format=zip --output protolang.zip master
```

TODO:

## 语义层面

- (√)变量的顺序问题
- 函数的顺序问题
- 参数不能和函数名字一样
- 非组合类型到底需不需要全局标识符？怎么判断类型相等？
- (√)return 得check一下吧，先支持return 再说
- (√)没有return也得检查
- 重名问题，现在签名完全一样的函数要调用到才会发现。。
- 处理省略type和init的bug.
- 处理test3的bug
- (x)处理正负号的优先级
- to、as、is 三个运算符 
- (√)bug: 可以定义名为long的函数？！
- 未初始化变量


## 关于const的经验

情况1：get_xxx 返回的是简单的数据结构，可以标记为const
情况2：get_xxx 返回的是一个指针，指向一个较为复杂或抽象的 class,
就不要返回const T*。因为const T*往往啥都干不了。
另外，这种函数也不要标记为const成员函数。因为有可能需要返回this出去，
this是const是塞不进非const的返回类型的。此外还要考虑到比较复杂的
返回类型，可能需要cache一下计算结果，下次直接返回cache。这时写入
cache也是const做不到的。

## Clion 坑人

不要选择上面的config preset，选下面的build preset!

## Requirements

- LLVM: 设置好你的

## 编译lld

不要投机取巧，省略部件，不然会后悔的捏

$ mkdir build
$ cd build
$ cmake -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DLLVM_ENABLE_PROJECTS="lld" ..
$ cmake --build . --config Release

## 关于编码的一些结论

- llvm::raw_fd_ostream 的构造函数需要 utf-8 编码的参数。
- std::filesystem::path 的构造函数需要 ANSI 编码的参数。
- glob::glob 里需要ANSI编码的参数
