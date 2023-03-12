打包

```
git archive --format=zip --output protolang.zip master
```

TODO:

## 语义层面

- 变量的顺序问题
- 参数不能和函数名字一样
- 非组合类型到底需不需要全局标识符？怎么判断类型相等？
- return 得check一下吧，先支持return 再说
- Error机制得重新整改