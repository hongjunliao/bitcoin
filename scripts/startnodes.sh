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

filename=$1

while IFS=: read -r _ dir || [[ -n $line ]]; do
#	mkdir ./build/$dir
#	ln -sf `pwd`/addr.txt `pwd`/build/$dir/
#	cp -t `pwd`/build/$dir/ bitcoin.conf
#	cp -t `pwd`/build/$dir/ addr.dat  blkindex.dat  wallet.dat
	if [ -d "build/$dir/" ]; then
	    echo "./build/bitcoin-v0.1.5 --datadir "build/$dir/""
	    ./build/bitcoin-v0.1.5 --datadir "build/$dir/"
	else
	    echo "init build/$dir/ first"
	fi

done < "$filename"


#line="path/to/directory:subdirectory"
#
#IFS=":" read -ra parts <<< "$line"
#dir="${parts[1]}"
#
#echo "The second part of the line is: $dir"