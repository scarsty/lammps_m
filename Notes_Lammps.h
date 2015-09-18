Lammps代码笔记

命名为.h文件是为编辑器使用cpp的语法高亮。

[]表示标题。

[c++风格]
模板比较少，主要是大量虚继承。推测一个命令对应一个类。类似的命令均继承基类。

[执行顺序]
lammps->input->file(); 依据文件中写的命令执行。
在一个死循环中调用
    if (execute_command()) {
      char *str = new char[maxline+32];
      sprintf(str,"Unknown command: %s",line);
      error->all(FLERR,str);
}
其中包含大部分命令定义，不在定义中的命令通过头文件style_command.h查找。

  if (command_map->find(command) != command_map->end()) {
    CommandCreator command_creator = (*command_map)[command];
    command_creator(lmp,narg,arg);
    return 0;
  }
在其中找到run，就可以找到主循环位置。

[stub]
stub库的目的是伪造一个MPI制造串行编译。

[在VS下面的编译方法]
erf.h文件中删除erf和erfc。
//double erf(double x);
//double erfc(double x);

删除erf.c文件。

Pointer类下面写了大量其子类的指针，需加上class关键字。

ftruncate全部改成_chsize。
部分int index[p+2]的写法换掉。
少数头文件定义不全，补上。
不存在的头文件包含全部去掉。

3个基本库是KSPACE，MOLUCULE，MANYBODY。

[重点放在fix命令上，有可能还包含compute]
后面的参数arg，似乎是一个字串数组。
0 fix id。
1 groupname，group的最大数量似乎是32。
std::map<std::string,FixCreator> *fix_map;
fix_map保存了fix类全部命令类型。定义在Modify类型中。
在Modify的构造函数中，按照style_fix.h中的包含来填充。
类似style_xxx.h的文件，在linux下面，由Make.sh style命令生成。在Windows下面则是预先写好。
因此写一个新类型直接放进去就可以。

[atom类]
double **x,**v,**f;
这3个看起来是坐标，速度，力。
  
[fix setwhnvelocity]
从fix setforce经过修改而来。

