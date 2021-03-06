#! /bin/bash

# 获取当前 shell script path
curPath=$(readlink -f "$(dirname "$0")")
for file in `ls $curPath` #注意此处这是两个反引号，表示运行系统命令
do
    # 如果当前是文件夹
    if [ -d $curPath"/"$file ] 
    then
        # 进入文件夹
        echo "[+] change directory to $curPath"/"$file"
        cd $curPath"/"$file;
        
        # 删除旧的 dot图以及 bc，ll 等
        rm *.dot *.ll *.bc
        
        # 生成新的 bc 和 ll，注意只针对 C 语言的 example
        echo "[+] building the no_opt bc & ll ..."
        clang -c -fno-discard-value-names -emit-llvm -O0 -Xclang -disable-O0-optnone *.c -o test.bc
        llvm-dis test.bc -o test.ll 

        # 生成新图
        echo "[+] printing ICFG ..."
        wpa -type -dump-icfg -stat=false test.bc 
        
        echo "[+] printing CFG ..."
        opt -dot-cfg test.bc

        echo "[+] printing PostDominator Tree ..."
        opt --dot-postdom test.bc

        # 返回上一级文件夹
        cd .. 
    fi
done
