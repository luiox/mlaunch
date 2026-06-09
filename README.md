# mvdbc
dbc文件生成C的打包解包代码工具

跟cantools的代码差不多，都拆分encode、decode、pack、unpack的架构，但是我这个要user提供一个白名单，只生成这部分的，剩下不管。我打算用cpp，因为https://github.com/nberlette/vector_dbc这个是cpp实现的，gui选qt6
