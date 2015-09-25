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

[一个timestep的过程]
主要引自开发文档。

ev_set()
fix->initial_integrate()
fix->post_integrate()

nflag = neighbor->decide()
if nflag:
	fix->pre_exchange()
	domain->pbc()
	domain->reset_box()
	comm->setup()
	neighbor->setup_bins()
	comm->exchange()
	comm->borders()
	fix->pre_neighbor()
	neighbor->build()
else
	comm->forward_comm()

force_clear()
fix->pre_force()

pair->compute()
bond->compute()
angle->compute()
dihedral->compute()
improper->compute()
kspace->compute()

comm->reverse_comm()

fix->post_force()
fix->final_integrate()
fix->end_of_step()

if any output on this step: output->write()

[stub]
stub库的目的是伪造一个MPI制造串行编译。

[在VS下面的编译方法]
erf.h文件中删除erf和erfc。
//double erf(double x);
//double erfc(double x);

类似下面的语句添加const
const unsigned int maxlib = 1024;

删除erf.c文件。

Pointer类下面写了大量其子类的指针，需加上class关键字。

ftruncate全部改成_chsize。
可以在fix.h中增加：
#ifdef _MSC_VER
#include <io.h>
#define ftruncate _chsize
#endif

部分int index[p+2]的写法换掉。
少数文件中有未定义类型，是因为头文件包含不全，补上。
不存在的头文件包含全部去掉。
power函数两个参数全部强制成double。

3个基本库是KSPACE，MOLUCULE，MANYBODY。

[重点放在fix命令上，有可能还包含compute]
后面的参数arg，似乎是一个字串数组。
0 fix id。
1 groupname，group的最大数量似乎是32。
std::map<std::string,FixCreator> *fix_map;
fix_map保存了fix类全部命令类型。定义在Modify类型中。
在Modify的构造函数中，按照style_fix.h中的包含来填充。
类似style_xxx.h的文件，在linux下面由Make.sh style命令生成，而在Windows下面目前来看是预先写好，可能也是自动生成的。
因此写一个新类型直接放进去就可以。
以下宏用于添加命令进入fix列表，范例为fix setforce命令。
FixStyle(setforce,FixSetForce)

[atom类]
double **x,**v,**f;
这3个看起来是坐标，速度，力。
  
[fix setwhnvelocity]
从fix setforce经过修改而来。
代码待处理。

