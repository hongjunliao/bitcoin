#/*!
# * @author hongjun.liao <docici@126.com>, @date 2023/7/15
# *
# * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
# * */
#
#// Copyright (c) 2009 Satoshi Nakamoto
#// Distributed under the MIT/X11 software license, see the accompanying
#// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#
#/////////////////////////////////////////////////////////////////////////////////////////////
#!/bin/bash

loc="$0"
filename=$1
n_chld=0

# 定义信号处理函数
cleanup() {
    echo "$loc: 收到终止信号，正在终止子进程..."
    pkill -P $$  # 终止所有由当前脚本启动的子进程
    exit 0
}

# 捕获终止信号并调用处理函数
trap cleanup EXIT

################################################################################################
echo "" > test/nodes.log
while IFS=: read -r _ dir || [[ -n $line ]]; do
	[ -d "`pwd`/test/$dir" ] || mkdir `pwd`/test/$dir
	ln -sf `pwd`/addr.txt `pwd`/test/$dir/
	[ -f "`pwd`/test/$dir/bitcoin.conf" ] || cp -t `pwd`/test/$dir/ `pwd`/bitcoin.conf
#	[ -f "`pwd`/test/$dir/blkindex.dat" ] || cp -t `pwd`/test/$dir/ `pwd`/addr.dat  `pwd`/blkindex.dat  `pwd`/wallet.dat
	if [ -d "test/$dir/" ]; then
	    echo "$loc: ./build/bitcoin-v0.1.5 --datadir "test/$dir/""
	    ./build/bitcoin-v0.1.5 --datadir "test/$dir/" >> test/nodes.log  2>&1 &
	    ((n_chld++))
	else
	    echo "$loc: init test/$dir/ first"
	fi

done < "$filename"

################################################################################################
echo "$loc: waiting for $n_chld childs to exit ..."
wait

#line="path/to/directory:subdirectory"
#
#IFS=":" read -ra parts <<< "$line"
#dir="${parts[1]}"
#
#echo "The second part of the line is: $dir"
