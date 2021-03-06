# Control Dependence Graph (CDG) 控制依赖图 构建算法

> 注意：该项目依赖 [SVF库](https://github.com/SVF-tools/SVF)
> 
> 需要提前设置环境变量 LLVM_DIR 以及 SVF_DIR

- 使用以下命令编译 Debug 版

  ```bash
  npm i svf-lib
  cmake . && make
  ```

- 使用以下命令编译 Release 版

  ```bash
  npm i svf-lib
  cmake -DCMAKE_BUILD_TYPE=Debug . && make
  ```

- 执行以下脚本以生成 example 中调试所需要的 bc、ll、CFG、ICFG 以及 PostDominator Tree

  ```bash
  ./examples/print_test_graph.sh
  ```